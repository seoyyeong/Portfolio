#pragma once
#include "pch.h"


#define dfLOG_MAX_LEN 1024
#define dfLOG_LEVEL_TYPECNT 4
#define __LOG_FILE

#ifdef __LOG_FILE
#define _LOG(szType, LogLevel, fmt, ...) CLog::Log(szType, LogLevel, fmt, __VA_ARGS__);
#define _LOGHEX(szType, LogLevel, fmt, ...)	CLog::LogHex(szType, LogLevel, fmt, __VA_ARGS__);	
#else
#define _LOG(szType, LogLevel, fmt, ...)
#define _LOGHEX(szType, LogLevel, fmt, ...)
#endif

#define _SET_LOGLEVEL(Level) CLog::SetLogLevel(Level);


class CLog
{
public:

	enum en_LOG_LEVEL
	{
		LEVEL_DEBUG = 0,
		LEVEL_WARNING,
		LEVEL_ERROR,
		LEVEL_SYSTEM,
	};

	static CLog* GetInstance(void);
	static void  Destroy(void);

	static void Log(const WCHAR* szType, int LogLevel, const WCHAR* szStringFormat, ...);

	static void SetLevel(int iLevel);
	static void SetLevel(const WCHAR* pLevel);

	static void PrintRunTimeLog(void);

private:

	CLog(void);
	~CLog(void) { Destroy(); };

	static CLog*            pInst;
	static std::mutex       Mutex;
	static CRITICAL_SECTION LogLock;

	int     iLogLevel;
	int     iLogCount;
	WCHAR   FilePath[MAX_PATH];

	time_t Start;

};
