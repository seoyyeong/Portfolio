#pragma once

#include "pch.h"


#define __PROFILE

#ifdef __PROFILE
#define PRO_BEGIN(TagName)	CTlsProfile::BeginProfile(TagName)
#define PRO_END(TagName)	CTlsProfile::EndProfile(TagName)
#else
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif

#define MAX_PROFILE_FUNC 20 //������� �������ϸ��� �Լ� �ִ� ����
#define ROW_LEN 22
#define LINE_LEN ROW_LEN*5+1


//�̱��� ��Ƽ������ �������Ϸ�

class CTlsProfile
{
public:

	//���� �������ϸ� �����Ͱ� �� ����ü
	struct PROFILE_SAMPLE
	{
		long			lFlag;				// ���������� ��� ����. 0 �̻�� / 1 ���
		char			szName[64];			// �������� ���� �̸�.

		LARGE_INTEGER	lStartTime;			// �������� ���� ���� �ð�.

		double			dTotalTime;			// ��ü ���ð� ī���� Time.	(��½� ȣ��ȸ���� ������ ��� ����)
		double			dMin[2];			// �ּ� ���ð� ī���� Time.	(�ʴ����� ����Ͽ� ���� / [0] �����ּ� [1] ���� �ּ� [2])
		double			dMax[2];			// �ִ� ���ð� ī���� Time.	(�ʴ����� ����Ͽ� ���� / [0] �����ִ� [1] ���� �ִ� [2])

		__int64			iCall;				// ���� ȣ�� Ƚ��.
		DWORD		    dwThreadID;			// ������ ID ����(�����)

		PROFILE_SAMPLE()
		{
			lFlag = 0;
			QueryPerformanceCounter(&lStartTime);
			dTotalTime = 0;
			dMin[0] = 0x7FFFFFFF'FFFFFFFF;
			dMin[1] = 0x7FFFFFFF'FFFFFFFF;
			dMax[0] = 0;
			dMax[1] = 0;
			iCall = 0;
			dwThreadID = GetCurrentThreadId();
		}

	};

	struct PROFILE_OUT
	{
		char szName[64];
		double dTotal;
		double dMin;
		double dMax;
		__int64 iCall;

		PROFILE_OUT()
		{
			dTotal = 0;
			dMin = 0x7FFFFFFF'FFFFFFFF;
			dMax = 0;
			iCall = 0;
		}

	};

	static CTlsProfile* GetInstance(void);

	static void         ReleaseProfile();

	static bool BeginProfile(const char* tag);
	static bool EndProfile(const char* tag);


	static void ProfileDataOutText(void);

	static void ProfileReset(void);

private:

	CTlsProfile();
	CTlsProfile(const CTlsProfile& ref) {};
	~CTlsProfile() {};

	static CTlsProfile*			 _pInst;
	static std::mutex            _Mutex;
	static CRITICAL_SECTION		 _ProfileLock;
	static DWORD			     _dwTlsIndex;
	static std::list<PROFILE_SAMPLE*> _ThreadList;
};