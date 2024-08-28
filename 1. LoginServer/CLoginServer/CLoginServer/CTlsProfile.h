#pragma once

#include "pch.h"


//#define __PROFILE

#ifdef __PROFILE
#define PRO_BEGIN(TagName)	CTlsProfile::BeginProfile(TagName)
#define PRO_END(TagName)	CTlsProfile::EndProfile(TagName)
#else
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#endif

#define MAX_PROFILE_FUNC 20 //스레드당 프로파일링할 함수 최대 개수
#define ROW_LEN 22
#define LINE_LEN ROW_LEN*5+1


//싱글톤 멀티스레드 프로파일러

class CTlsProfile
{
public:

	//실제 프로파일링 데이터가 들어갈 구조체
	struct PROFILE_SAMPLE
	{
		long			lFlag;				// 프로파일의 사용 여부. 0 미사용 / 1 사용
		char			szName[64];			// 프로파일 샘플 이름.

		LARGE_INTEGER	lStartTime;			// 프로파일 샘플 실행 시간.

		double			dTotalTime;			// 전체 사용시간 카운터 Time.	(출력시 호출회수로 나누어 평균 구함)
		double			dMin[2];			// 최소 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최소 [1] 다음 최소 [2])
		double			dMax[2];			// 최대 사용시간 카운터 Time.	(초단위로 계산하여 저장 / [0] 가장최대 [1] 다음 최대 [2])

		__int64			iCall;				// 누적 호출 횟수.
		DWORD		    dwThreadID;			// 스레드 ID 저장(예비용)

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

	//lock, 싱글톤 인스턴스, QueryPerformanceFreq 초기화
	static void*		InitProfile();
	static void         ReleaseProfile();

	/////////////////////////////////////////////////////////////////////////////
	// 하나의 함수 Profiling 시작, 끝 함수.
	//
	// Parameters: (char *)Profiling이름.
	// Return: 없음.
	/////////////////////////////////////////////////////////////////////////////
	static bool BeginProfile(const char* tag);
	static bool EndProfile(const char* tag);

	/////////////////////////////////////////////////////////////////////////////
	// Profiling 된 데이타를 Text 파일로 출력한다.
	//
	// Parameters: (char *)출력될 파일 이름.
	// Return: 없음.
	/////////////////////////////////////////////////////////////////////////////
	static void ProfileDataOutText(void);

	/////////////////////////////////////////////////////////////////////////////
	// 프로파일링 된 데이터를 모두 초기화 한다.(프로세스 단위)
	//
	// Parameters: 없음.
	// Return: 없음.
	// 
	// 주의사항 : 만약 프로파일링 데이터를 쓰다가(beginprofile, endprofile 또는 Dataout 도중...)
	//           이 함수에 진입했을 경우에는 다른 스레드의 동작 중 데이터가 오염될 수 있습니다.
	//           
	/////////////////////////////////////////////////////////////////////////////
	static void ProfileReset(void);

private:

	CTlsProfile();
	CTlsProfile(const CTlsProfile& ref) {};
	~CTlsProfile() {};

	static CTlsProfile*			 _pInst;
	static CRITICAL_SECTION		 _ProfileLock;
	static DWORD			     _dwTlsIndex;
	static std::list<PROFILE_SAMPLE*> _ThreadList;
};