
#include "pch.h"
#include "CLog.h"

CLog* CLog::pInst;
std::mutex CLog::Mutex;
CRITICAL_SECTION CLog::LogLock;

const WCHAR* g_LevelLookup[dfLOG_LEVEL_TYPECNT] = { L"DEBUG", L"WARNING", L"ERROR", L"SYSTEM" };

CLog::CLog(void)
{
	InitializeCriticalSection(&LogLock);

	iLogLevel = LEVEL_DEBUG;
	iLogCount = 0;

	wcscpy_s(FilePath, L"../Log");
	_wmkdir(FilePath);

	Start = time(NULL);
}


CLog* CLog::GetInstance(void)
{
	if (pInst == nullptr)
	{
		Mutex.lock();
		if (pInst == nullptr)
		{
			pInst = new CLog;
		}
		Mutex.unlock();
	}
	return pInst;
}

void CLog::Destroy(void)
{
	delete pInst;
}

void CLog::Log(const WCHAR* szType, int LogLevel, const WCHAR* szStringFormat, ...)
{
	FILE*   pFile;
	WCHAR   szBuffer[dfLOG_MAX_LEN];
	WCHAR   szFileName[MAX_PATH];
	WCHAR*  FilePath;
	WCHAR*  p;

	va_list va;
	int     len;
	WCHAR   szLevel[20];

	time_t timer;
	tm     t;

	CLog* pInst = GetInstance();

	timer = time(NULL);
	localtime_s(&t, &timer);
	FilePath = pInst->FilePath;

	if (pInst->iLogLevel > LogLevel)
	{
		return;
	}

	switch(LogLevel)
	{
		case LEVEL_DEBUG:
			wcscpy_s(szLevel, L"DEBUG");
			break;
		case LEVEL_WARNING:
			wcscpy_s(szLevel, L"WARNING");
			break;
		case LEVEL_ERROR:
			wcscpy_s(szLevel, L"ERROR");
			break;
		case LEVEL_SYSTEM:
			wcscpy_s(szLevel, L"SYSTEM");
			break;
		default:
			swprintf_s(szLevel, L"UNDEFINED : %d", LogLevel);
			break;
	}

	len = swprintf_s(szBuffer, dfLOG_MAX_LEN * sizeof(WCHAR),
		  L"[%s] [%04d-%02d-%02d %02d:%02d:%02d / %s / %010d ] ",
		  szType,
		  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
		  szLevel, InterlockedIncrement((long*)&pInst->iLogCount));

	p = szBuffer + len;

	va_start(va, szStringFormat);
	StringCchVPrintf(p, len, szStringFormat, va);
	va_end(va);

	//파일 이름
	swprintf_s(szFileName, MAX_PATH, L"%s/%04d%02d_%s.txt",
		pInst->FilePath, t.tm_year + 1900, t.tm_mon + 1, szType);

	EnterCriticalSection(&pInst->LogLock);

	_wfopen_s(&pFile, szFileName, L"a+");
	fwprintf_s(pFile, L"%s\n", szBuffer);
	fclose(pFile);

	LeaveCriticalSection(&pInst->LogLock);

}

void CLog::LogHex(const WCHAR* szType, int LogLevel, const WCHAR* szLog, BYTE* pByte, int iByteLen)
{
	CLog* pInst = GetInstance();

	if (pInst->iLogLevel > LogLevel)
	{
		return;
	}

	EnterCriticalSection(&pInst->LogLock);
	//AcquireSRWLockExclusive(&pInst->LogLock);


	LeaveCriticalSection(&pInst->LogLock);
	//ReleaseSRWLockExclusive(&pInst->LogLock);
}

void CLog::SetLevel(int iLevel)
{
	CLog* pInst = GetInstance();

	pInst->iLogLevel = iLevel;
}

void CLog::SetLevel(const WCHAR* pLevel)
{
	CLog* pInst = GetInstance();

	for (int i = 0; i < dfLOG_LEVEL_TYPECNT; i++)
	{
		if (wcscmp(pLevel, g_LevelLookup[i]) == 0)
		{
			pInst->iLogLevel = i;
		}
	}
	
}

void CLog::PrintRunTimeLog(void)
{
	CLog* pInst = GetInstance();
	time_t timer;
	tm     t;

	timer = time(NULL); // 1970년 1월 1일 0시 0분 0초부터 시작하여 현재까지의 초
	localtime_s(&t, &timer); // 포맷팅을 위해 구조체에 넣기

	wprintf(L"[%04d/%02d/%02d, %02d:%02d:%02d] ",
		t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
		t.tm_hour, t.tm_min, t.tm_sec);

	int cur = timer - pInst->Start;
	int day = cur / 60 / 60 / 24;
	cur -= day * 60 * 60 * 24;
	int hour = cur / 60 / 60;
	cur -= hour * 60 * 60;
	int min = cur / 60;
	cur -= min * 60;
	int sec = cur;
	wprintf(L"Running Time : %dd %dh %dm %ds\n\n", day, hour, min, sec);
}