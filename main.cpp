#include <iostream>
#include "media.h"
#include "mediasource.h"
using namespace std;


/*001 ���Զ�������������Ƕ�������Զ�����������:�Ƚ����ݳ�Ա�ĳ�ʼ��˳��
**    (1) �����������ܴ�����
**    (2) ���ݳ�Ա�����ඨ���е�˳����г�ʼ��
**    (3) Ƕ�����Ĺ��캯����ִ�ж���Ĺ��캯����ǰ������
**    (4) ��ͬǶ�����Ĺ���˳�������ǵ�˳�����
**    (5) Ƕ����󰴹������ǵ��෴˳������
*/
#if 0
class Matter
{
public:
	Matter(int idx):m_id(idx)
	{
		cout << m_id << ":" << "This is a matter!\n";
	}

	~Matter()
	{
		cout << m_id << ":" << "matter destructor!\n";
	}

private:
	const int m_id;
};


class World
{
public:
	World(int idx):m_Matter(idx),m_identifier(idx)
	{
		cout << m_identifier << ":" << "Hello!" << endl;
	}

	~World()
	{
		cout << m_identifier << ":" << "Good Bye!" << endl;
	}

	Matter m_Matter;

private:
	const int m_identifier;
};

World TheWorld(1);
#endif


/*002 ��ļ̳�: �Ƚϻ����������Ĺ��캯�������������ĵ���˳��
**    (1)����Ĺ��캯�����������౻����
**    (2)������������������ڻ��౻����
*/
#if 0
class CelestialBody
{
public:
	CelestialBody(float mass):m_mass(mass)
	{
		cout << "CelestialBody Constructor from " << m_mass << endl;
	}

	~CelestialBody()
	{
		cout << "CelestialBody Destructor from " << m_mass << endl;
	}

private:
	const float m_mass;
};


class Star:public CelestialBody
{
public:
	Star(float mass,float brightness):CelestialBody(mass),m_brightness(brightness)
	{
		cout << "Star Constructor from " << m_brightness << endl;
	}

	~Star()
	{
		cout << "Star Destructor from " << m_brightness << endl;
	}

private:
	const float m_brightness;
};
#endif


/*003 ��Ա�����ͽӿ�,��Ա������������
**    (1)ʹ�ó�Ա�����������ݳ�Աû������ʱ����,����û��̫�������ڳ�����ʹ�ù������ݳ�Ա
*/
#if 0
class InputNum
{
public:
	InputNum(char aString[])
	{
		cout << aString << endl;
		cin >> m_num;
	}

	int GetValue()
	{
		return m_num;
	}

	void AddNum(char bString[])
	{
		InputNum aInputNum(bString);
		m_num += aInputNum.GetValue();
	}

	~InputNum()
	{
		cout << "destructor from " << m_num << endl;
	}

private:
	int m_num;
};
#endif


/*004 
**
*/

int main()
{
	MediaSource* pMediaSource = new MediaSource(1);
	cout << "Here is Main function!\n";
	pMediaSource->Print();
	delete pMediaSource;
    return 0;
}



