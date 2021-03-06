#include <iostream>
#include <fstream>
#include <string.h>  /* needed by sscanf */
#include <string>
#include <vector>
#include "getopt.hpp"
#include "common.h"
#include "yuv_process.h"

using namespace std;

#ifdef _WIN32
#define SSCANF sscanf_s
#else
#define SSCANF sscanf
#endif

static void draw_red_dot(YuvInfo *yuv, SadInfo *info)
{
    RectangleInfo rec_info;
    rec_info.left = info->mb_x * info->mb_size;
    rec_info.top = info->mb_y * info->mb_size;
    rec_info.right = info->mb_x * info->mb_size + 1;
    rec_info.bottom = info->mb_y * info->mb_size + 1;
    rec_info.y_pixel = 76; /* Red */
    rec_info.u_pixel = 84;
    rec_info.v_pixel = 255;

    draw_rectangle(yuv, &rec_info);
}

static void draw_blue_dot(ProcCtx *ctx, vector<Rect> &rects)
{
    YuvInfo *yuv = &ctx->yuv_info;
    RectangleInfo rec_info;
    vector<Rect>::iterator iter;

    for (iter = rects.begin(); iter != rects.end(); iter++) {

        rec_info.left = iter->left;
        rec_info.top = iter->top;
        rec_info.right = iter->left + 1;
        rec_info.bottom = iter->top + 1;
        rec_info.y_pixel = 29; /* Blue */
        rec_info.u_pixel = 255;
        rec_info.v_pixel = 107;

        draw_rectangle(yuv, &rec_info);
    }
}

static bool circle_state(char *buf, uint32_t stride, uint32_t pos)
{
    if (buf[pos - stride] && buf[pos + stride] &&
        buf[pos - 1] && buf[pos - 1 - stride] && buf[pos - 1 + stride] &&
        buf[pos + 1] && buf[pos + 1 - stride] && buf[pos + 1 + stride]) {
        return true;
    }
    return false;
}

static void set_motion_blocks(ProcCtx *ctx, char *buf, uint32_t buf_len)
{
    uint32_t mb_width = ctx->width / ctx->mb_size;
    uint32_t mb_height = ctx->height / ctx->mb_size;
    uint32_t buf_stride = mb_width + 2;
    uint32_t pos;
    char *p = buf + buf_stride + 1;

    for (uint32_t row = 0; row < mb_height; row++) {
        for (uint32_t col = 0; col < mb_width; col++) {
            pos = col + row * buf_stride;
            if (circle_state(buf, buf_stride, pos)) {
                p[pos] = 1;
            }
        }
    }
}

static void translate_coordinate(ProcCtx *ctx, char *buf, vector<Rect> &rects_fill)
{
    uint32_t mb_width = ctx->width / ctx->mb_size;
    uint32_t mb_height = ctx->height / ctx->mb_size;
    uint32_t buf_stride = mb_width + 2;
    uint32_t pos;
    char *p = buf + buf_stride + 1;

    for (uint32_t row = 0; row < mb_height; row++) {
        for (uint32_t col = 0; col < mb_width; col++) {
            pos = col + row * buf_stride;
            if (p[pos] == 1) {
                uint32_t left = col * ctx->mb_size;
                uint32_t top = row * ctx->mb_size;
                uint32_t right = left + ctx->mb_size;
                uint32_t bottom = top + ctx->mb_size;
                Rect rect(left, top, right, bottom);
                rects_fill.push_back(rect);
            }
        }
    }

}

static void fill_motion_blocks(ProcCtx *ctx, const vector<Rect> &rects, vector<Rect> &rects_fill)
{
    uint32_t idx, x_filled, y_filled;
    uint32_t mb_width = ctx->width / ctx->mb_size;
    uint32_t mb_height = ctx->height / ctx->mb_size;
    uint32_t buf_stride = mb_width + 2;
    uint32_t buf_len = (mb_width + 2) * (mb_height + 2);
    char *buf = new char[buf_len];
    Rect rect;
    memset(buf, 0, sizeof(char) * buf_len);

    for (idx = 0; idx < rects.size(); idx++) {
        rect = rects.at(idx);
        x_filled = rect.left / ctx->mb_size + 1;
        y_filled = rect.top / ctx->mb_size + 1;
        buf[x_filled + y_filled * buf_stride] = 1; /* motional block */
    }

    uint32_t pos = (mb_height + 1) * buf_stride;
    for (idx = 1; idx <= mb_width; idx++) {
        buf[idx] = buf[idx + buf_stride]; /* y_filled = 0 */
        buf[idx + pos] = buf[idx + pos - buf_stride]; /* y_filled = mb_height + 1 */
    }

    for (idx = 0; idx <= mb_height + 1; idx++) {
        buf[idx * buf_stride] = buf[idx * buf_stride + 1]; /* x_filled = 0 */
        buf[idx * buf_stride + buf_stride - 1] = buf[idx * buf_stride + buf_stride - 2]; /* x_filled = mb_width + 1 */
    }

    set_motion_blocks(ctx, buf, buf_len);
    translate_coordinate(ctx, buf, rects_fill);

    delete [] buf;
}

static void rk_handle_md(ProcCtx *ctx, ifstream *sad)
{
    YuvInfo *yuv = &ctx->yuv_info;
    SadInfo *info = &ctx->sad_info;
    uint32_t frame_num = ctx->frame_read;
    vector<Rect> rects, rects_new;
    Rect rect;
    char lines[512];
    if (frame_num == 1 && sad->getline(lines, 512)) {
        int match_cnt = SSCANF(lines, "frame=%d mb_size=%d mb_width=%d mb_height=%d mb_x=%d mb_y=%d",
                               &info->frame_cnt, &info->mb_size, &info->mb_width, &info->mb_height, &info->mb_x, &info->mb_y);
        if (match_cnt > 1) {
            cout << "match_cnt " << match_cnt << " frame_cnt " << info->frame_cnt
                 << " mb_width " << info->mb_width << " mb_height " << info->mb_height
                 << " mb_x " << info->mb_x << " mb_y " << info->mb_y << endl;

            ctx->mb_size = info->mb_size;
        }
    }

    while (info->frame_cnt == frame_num) {
        draw_red_dot(yuv, info);

        rect.left = info->mb_x * info->mb_size;
        rect.top = info->mb_y * info->mb_size;
        rect.right = rect.left + info->mb_size;
        rect.bottom = rect.top + info->mb_size;
        rect.motion_rate = 100;
        rect.area = 16;
        rects.push_back(rect);

        if (sad->getline(lines, 512)) {
            int match_cnt = SSCANF(lines, "frame=%d mb_size=%d mb_width=%d mb_height=%d mb_x=%d mb_y=%d",
                                   &info->frame_cnt, &info->mb_size, &info->mb_width, &info->mb_height, &info->mb_x, &info->mb_y);
            if (match_cnt > 1) {
                //cout << "match_cnt " << match_cnt << " frame_cnt " << info->frame_cnt
                //     << " mb_width " << info->mb_width << " mb_height " << info->mb_height
                //     << " mb_x " << info->mb_x << " mb_y " << info->mb_y << endl;
            }
        } else {
            cout << "No sad info now, exit!" << endl;
            break;
        }
    }

    /* merge motion macroblocks and draw blue rectangles */
    if (ctx->draw_blue_rect) {
        int64_t start, end;
        double duration;
        start = time_usec();
        cout << "frame_num " << frame_num << " Vector Rect Number " << rects.size() << endl;

        /* simplest merge, low efficiency */
        //merge_rect((void *)ctx, rects);

        /* optimized merge, middle efficiency */
        merge_rect_optimize((void *)ctx, rects, rects_new);

        end = time_usec();
        duration = (double)(end - start) / 1000000;
        cout << "frame_num " << frame_num << " finish merge, " << duration << " seconds" << endl;

        draw_blue_rectangle(yuv, rects_new);
    }

    if (ctx->draw_blue_dot) {
        vector<Rect> rects_fill;
        fill_motion_blocks(ctx, rects, rects_fill);
        draw_blue_dot(ctx, rects_fill);
    }
}

static void process_yuv(ProcCtx *ctx)
{
    uint32_t luma_size = ctx->width * ctx->height;
    uint32_t chroma_size = luma_size / 2;
    uint32_t frame_size = luma_size + chroma_size;
    uint32_t left = ctx->left;
    uint32_t top = ctx->top;
    uint32_t right = ctx->right;
    uint32_t bottom = ctx->bottom;
    uint32_t frame_cnt, region_num, region_idx;
    ifstream coord(ctx->coord_file.c_str());
    ifstream sad_path(ctx->sad_file.c_str());
    YuvInfo *yuv_info = &ctx->yuv_info;
    RectangleInfo rec_info;
    char lines[512];
    char *buf = new char[frame_size];

    if (coord.getline(lines, 512)) {
        int match_cnt = SSCANF(lines, "frame=%d, num=%d, idx=%d, left=%d, top=%d, right=%d, bottom=%d",
                               &frame_cnt, &region_num, &region_idx, &left, &top, &right, &bottom);
        if (match_cnt > 1) {
            cout << "match_cnt " << match_cnt << " frame_cnt " << frame_cnt
                 << " region_num " << region_num << " region_idx " << region_idx
                 << " left " << left << " top " << top
                 << " right " << right << " bottom " << bottom << endl;
        }
    }

    ctx->ifs->read(buf, frame_size);

    do {
        while (ctx->frame_read == frame_cnt) {
            yuv_info->buf = buf;
            yuv_info->width = ctx->width;
            yuv_info->height = ctx->height;
            rec_info.left = left;
            rec_info.top = top;
            rec_info.right = right;
            rec_info.bottom = bottom;
            rec_info.y_pixel = 76; /* Red */
            rec_info.u_pixel = 84;
            rec_info.v_pixel = 255;

            draw_rectangle(yuv_info, &rec_info);

            if (coord.getline(lines, 512)) {
                int match_cnt = SSCANF(lines, "frame=%d, num=%d, idx=%d, left=%d, top=%d, right=%d, bottom=%d",
                                       &frame_cnt, &region_num, &region_idx, &left, &top, &right, &bottom);
                if (match_cnt > 1) {
                    //cout << "match_cnt " << match_cnt << " frame_cnt " << frame_cnt
                    //     << " region_num " << region_num << " region_idx " << region_idx
                    //     << " left " << left << " top " << top
                    //     << " right " << right << " bottom " << bottom << endl;
                }
            } else {
                cout << "No MD info now, exit!" << endl;
                break;
            }
            cout << "finish frame " << frame_cnt
                 << " region " << region_idx << endl;
        }

        if (ctx->enable_draw_dot && ctx->frame_read > 0) {
            rk_handle_md(ctx, &sad_path);
        }

        ctx->ofs->write(buf, frame_size);

        ctx->ifs->read(buf, frame_size);
        ctx->frame_read++;
    } while (ctx->frame_read < ctx->frames);

    delete [] buf;
}

int main(int argc, char **argv)
{
    ProcCtx proc_ctx;
    ProcCtx *ctx = &proc_ctx;
    memset(ctx, 0, sizeof(ProcCtx));

    /* -c 3903.md --- Motion detected regions of Hisilicon
     * -s   *.sad --- Motion detected regions of RK
     */
    cout << "----------Test-------------" << endl;
    bool help = getarg(false, "-H", "--help", "-?");
    string in_file = getarg("F:\\rkvenc_verify\\input_yuv\\3903_720x576.yuv", "-i", "--input");
    string out_file = getarg("F:\\rkvenc_verify\\input_yuv\\3903_720x576_hi_rk.yuv", "-o", "--output");
    ctx->coord_file = getarg("F:\\rkvenc_verify\\cfg\\3903_hisilicon.md", "-c", "--coordinate");
    ctx->sad_file = getarg("F:\\rkvenc_verify\\cfg\\3903_720x576_150_1.sad", "-s", "--sad");
    ctx->width = getarg(720, "-w", "--width");
    ctx->height = getarg(576, "-h", "--height");
    ctx->left = getarg(10, "-l", "--left");
    ctx->top = getarg(20, "-t", "--top");
    ctx->right = getarg(50, "-r", "--right");
    ctx->bottom = getarg(80, "-b", "--bottom");
    ctx->frames = getarg(2, "-f", "--frames");
    ctx->motion_rate_thresh = getarg(50, "-m", "--motion_thresh");
    ctx->enable_draw_dot = getarg(1, "-dd", "--draw_dot");
    ctx->draw_blue_dot = getarg(1, "-dbd", "--draw_blue_dot");
    ctx->draw_blue_rect = getarg(0, "-dbr", "--draw_blue_rect");

    if (help) {
        cout << "Usage:" << endl
             << "./yuv_process -i=3903_720x576.yuv -o=3903_720x576_hi_rk.yuv "
             << "-c=3903.md -s=3903_720x576_150.sad -f=2"
             << endl;
        return 0;
    }

    cout << "input: " << in_file << endl
         << "output: " << out_file << endl;
    ctx->ifs = new ifstream(in_file.c_str(), ios::binary | ios::in);

    ofstream ofs;
    ctx->ofs = &ofs;
    ofs.open(out_file.c_str(), ios::binary | ios::out);

    process_yuv(ctx);

    if (ctx->ifs && ctx->ifs != &cin)
        delete ctx->ifs;
    ofs.close();

    cout << "----------End!-------------" << endl;
    //string str;
    //cin >> str;
    return 0;
}
