
#include "CTlsProfile.h"



std::list<CTlsProfile::PROFILE_SAMPLE*> CTlsProfile::_ThreadList;
CTlsProfile*     CTlsProfile::_pInst = (CTlsProfile*)InitProfile();
CRITICAL_SECTION CTlsProfile::_ProfileLock;
DWORD		     CTlsProfile::_dwTlsIndex;
LARGE_INTEGER                  TimeFreq;

CTlsProfile::CTlsProfile(void)
{
	_dwTlsIndex = TlsAlloc();

	if (_dwTlsIndex == TLS_OUT_OF_INDEXES)
	{
		int* p = nullptr;
		*p = 0;
	}
}


CTlsProfile* CTlsProfile::GetInstance(void)
{
	if (_pInst == nullptr)
	{
		EnterCriticalSection(&_ProfileLock);
		if (_pInst == nullptr)
		{
			_pInst = new CTlsProfile;
			atexit(ReleaseProfile);
		}
		LeaveCriticalSection(&_ProfileLock);
	}
	return _pInst;
}

void* CTlsProfile::InitProfile(void)
{
	QueryPerformanceFrequency(&TimeFreq);
	InitializeCriticalSection(&_ProfileLock);
	return nullptr;
}

void CTlsProfile::ReleaseProfile(void)
{
	if (_pInst == nullptr)
	{
		return;
	}

	ProfileDataOutText();

	EnterCriticalSection(&_ProfileLock);
	std::list<PROFILE_SAMPLE*>::iterator iter = _ThreadList.begin();

	for (; iter != _ThreadList.end();)
	{
		delete[] (*iter);
		iter = _ThreadList.erase(iter);
	}
	LeaveCriticalSection(&_ProfileLock);

	delete _pInst;
	_pInst = nullptr;

	TlsFree(_dwTlsIndex);
	DeleteCriticalSection(&_ProfileLock);
}

bool CTlsProfile::BeginProfile(const char* tag)
{
	PROFILE_SAMPLE* ProfileArr;
	PROFILE_SAMPLE* pProfile;
	LARGE_INTEGER   StartTime;
	int				idx;
	int				iEmptyIdx = -1;

	ProfileArr = (PROFILE_SAMPLE*)TlsGetValue(_dwTlsIndex);

	//TLS 첫 접근 시 array 할당
	if (ProfileArr == nullptr)
	{
		ProfileArr = new PROFILE_SAMPLE[MAX_PROFILE_FUNC];

		EnterCriticalSection(&_ProfileLock);
		CTlsProfile* p = GetInstance();
		p->_ThreadList.push_back(ProfileArr);
		LeaveCriticalSection(&_ProfileLock);
		TlsSetValue(_dwTlsIndex, ProfileArr);
	}

	//1. 해당 태그에 대해 BeginProfile 호출한 적 있을 경우
	for (idx = 0; idx < MAX_PROFILE_FUNC; idx++)
	{
		if (ProfileArr[idx].lFlag == 1)
		{
			if (strcmp(tag, ProfileArr[idx].szName) == 0)
			{
				QueryPerformanceCounter(&StartTime);
				ProfileArr[idx].lStartTime = StartTime;
				return true;
			}
		}
		else
		{
			iEmptyIdx = idx;
			break;
		}

	}

	if (iEmptyIdx == -1)
	{
		return false;
	}

	//2. 항목을 추가해야 할 경우
	pProfile = &ProfileArr[iEmptyIdx];

	pProfile->lFlag = 1;
	strcpy_s(pProfile->szName, tag);
	QueryPerformanceCounter(&StartTime);
	pProfile->lStartTime = StartTime;

	return true;
}

bool CTlsProfile::EndProfile(const char* tag)
{
	PROFILE_SAMPLE* ProfileArr;

	LARGE_INTEGER lEndTime;
	double       DeltaTime;


	ProfileArr = (PROFILE_SAMPLE*)TlsGetValue(_dwTlsIndex);

	//TLS 첫 접근 시 추가할 데이터가 없으므로 false 반환
	if (ProfileArr == nullptr)
	{
		return false;
	}

	for (int i = 0; i < MAX_PROFILE_FUNC; i++)
	{
		if (ProfileArr[i].lFlag == 1)
		{
			if (strcmp(tag, ProfileArr[i].szName) == 0)
			{
				QueryPerformanceCounter(&lEndTime);
				DeltaTime = ((double)(lEndTime.QuadPart - ProfileArr[i].lStartTime.QuadPart)) / 10;
				ProfileArr[i].dTotalTime += DeltaTime;

				//min
				if (ProfileArr[i].dMin[0] > ProfileArr[i].dMin[1])
				{
					if (ProfileArr[i].dMin[0] > DeltaTime)
						ProfileArr[i].dMin[0] = DeltaTime;
				}
				else if (ProfileArr[i].dMin[1] >= ProfileArr[i].dMin[0])
				{
					if (ProfileArr[i].dMin[1] > DeltaTime)
						ProfileArr[i].dMin[1] = DeltaTime;
				}

				//max
				if (ProfileArr[i].dMax[0] < ProfileArr[i].dMax[1])
				{
					if (ProfileArr[i].dMax[0] < DeltaTime)
						ProfileArr[i].dMax[0] = DeltaTime;
				}
				else if (ProfileArr[i].dMax[1] <= ProfileArr[i].dMax[0])
				{
					if (ProfileArr[i].dMax[1] < DeltaTime)
						ProfileArr[i].dMax[1] = DeltaTime;
				}

				ProfileArr[i].iCall++;

				return true;
			}
		}

	}

	return false;
}

void CTlsProfile::ProfileReset(void)
{
	PROFILE_SAMPLE* ProfileArr;

	EnterCriticalSection(&_ProfileLock);
	std::list<PROFILE_SAMPLE*>::iterator iter = _ThreadList.begin();

	for (; iter != _ThreadList.end(); iter++)
	{
		ProfileArr = (*iter);
		for (int i = 0; i < MAX_PROFILE_FUNC; i++)
		{
			ProfileArr[i].lFlag = 0;
			ProfileArr[i].dTotalTime = 0;
			ProfileArr[i].dMin[0] = 0x7FFFFFFF'FFFFFFFF;
			ProfileArr[i].dMin[1] = 0x7FFFFFFF'FFFFFFFF;
			ProfileArr[i].dMax[0] = 0;
			ProfileArr[i].dMax[1] = 0;
			ProfileArr[i].iCall = 0;
			ProfileArr[i].szName[0] = '\0';
		}
	}
	LeaveCriticalSection(&_ProfileLock);
}

void CTlsProfile::ProfileDataOutText(void)
{
	FILE*      pFile;
	char       FileName[MAX_PATH];
	char*	   FileBuf;
	char*      p;
	char       dtowBuff[16];

	SYSTEMTIME stNowTime;
	errno_t    FileErr;

	std::vector<PROFILE_OUT> OutData;
	PROFILE_OUT         Data;
	PROFILE_OUT*        pProfile;
	bool bFlag = false;

	//데이터 정리
	EnterCriticalSection(&_ProfileLock);
	std::vector<PROFILE_SAMPLE[MAX_PROFILE_FUNC]> ProfileArr(_ThreadList.size());

	std::list<PROFILE_SAMPLE*>::iterator iter = _ThreadList.begin();

	for (int i = 0; i < _ThreadList.size(); i++)
	{
		memcpy(ProfileArr[i], *iter, sizeof(PROFILE_SAMPLE) * MAX_PROFILE_FUNC);
		++iter;
	}
	LeaveCriticalSection(&_ProfileLock);

	for (int i = 0; i < ProfileArr.size(); i++)
	{
		for (int j = 0; j < MAX_PROFILE_FUNC; j++)
		{
			if (ProfileArr[i][j].lFlag == 1)
			{
				bFlag = false;
				for (int k = 0; k < OutData.size(); k++)
				{
					if (strcmp(OutData[k].szName, ProfileArr[i][j].szName) == 0)
					{
						//Outliar 데이터 각 4개씩 제외

						OutData[k].dTotal += ProfileArr[i][j].dTotalTime;
						if (ProfileArr[i][j].iCall > 4)
						{
							OutData[k].dTotal -= ProfileArr[i][j].dMax[0] + ProfileArr[i][j].dMax[1];
							OutData[k].dTotal -= ProfileArr[i][j].dMin[0] + ProfileArr[i][j].dMin[1];
						}
						OutData[k].dMax = max(OutData[k].dMax, max(ProfileArr[i][j].dMax[0], ProfileArr[i][j].dMax[1]));
						OutData[k].dMin = min(OutData[k].dMin, min(ProfileArr[i][j].dMin[0], ProfileArr[i][j].dMin[1]));
						
						OutData[k].iCall += ProfileArr[i][j].iCall;

						bFlag = true;
						break;
					}
				}
				if (bFlag == false)
				{
					strcpy_s(Data.szName, ProfileArr[i][j].szName);

					Data.dTotal = ProfileArr[i][j].dTotalTime;
					//Outliar 데이터 각 4개씩 제외
					if (Data.iCall > 4)
					{
						Data.dTotal -= ProfileArr[i][j].dMax[0] + ProfileArr[i][j].dMax[1];
						Data.dTotal -= ProfileArr[i][j].dMin[0] + ProfileArr[i][j].dMin[1];

					}
					Data.dMax = max(ProfileArr[i][j].dMax[0], ProfileArr[i][j].dMax[1]);
					Data.dMin = min(ProfileArr[i][j].dMin[0], ProfileArr[i][j].dMin[1]);
					Data.iCall = ProfileArr[i][j].iCall;


					OutData.push_back(Data);
				}
			}

		}
	}


	//파일 IO
	GetLocalTime(&stNowTime);

	_mkdir("../Profile");

	sprintf_s(
		FileName,
		"../Profile/Profile_%d%02d%02d_%02d.%02d.%02d.txt"
		, stNowTime.wYear
		, stNowTime.wMonth
		, stNowTime.wDay
		, stNowTime.wHour
		, stNowTime.wMinute
		, stNowTime.wSecond
	);

	FileErr = fopen_s(&pFile, FileName, "w");
	if (FileErr != 0)
	{
		return;
	}

	FileBuf = new char[(OutData.size() + 2) * LINE_LEN * OutData.size()];
	memset(FileBuf, ' ', (OutData.size() + 2) * LINE_LEN * OutData.size());
	p = FileBuf;

	strcpy_s(p, LINE_LEN, " Name               | Average             | Min                 | Max                 | Call                |\n");
	p += LINE_LEN;

	strcpy_s(p, LINE_LEN, "------------------------------------------------------------------------------------------------------------\n");
	p += LINE_LEN - 1;

	std::vector<PROFILE_OUT>::iterator viter = OutData.begin();
	for (; viter != OutData.end(); ++viter)
	{
		pProfile = &(*viter);
		//Name
		strcpy_s(p, strlen(pProfile->szName) + 1, pProfile->szName);
		p += ROW_LEN - 3;
		*p = L' ';
		p++;
		*p = L'|';
		p++;
		*p = L' ';
		p++;

		//Average

		sprintf_s(dtowBuff, "%.4lf㎲", pProfile->dTotal/pProfile->iCall);
		strcpy_s(p, strlen(dtowBuff) + 1, dtowBuff);
		p += ROW_LEN - 2;
		*p = L'|';
		p++;
		*p = L' ';
		p++;

		//Min

		sprintf_s(dtowBuff, "%.4lf㎲", pProfile->dMin);
		strcpy_s(p, strlen(dtowBuff) + 1, dtowBuff);
		p += ROW_LEN - 2;
		*p = L'|';
		p++;
		*p = L' ';
		p++;

		//Max
		sprintf_s(dtowBuff, "%.4lf㎲", pProfile->dMax);
		strcpy_s(p, strlen(dtowBuff) + 1, dtowBuff);
		p += ROW_LEN - 2;
		*p = L'|';
		p++;
		*p = L' ';
		p++;

		//Call
		sprintf_s(dtowBuff, "%lld", pProfile->iCall);
		strcpy_s(p, strlen(dtowBuff) + 1, dtowBuff);
		p += ROW_LEN - 2;
		*p = L'\n';
		p++;
		*p = L' ';
		p++;

	}

	fwrite(FileBuf, (OutData.size() + 2) * LINE_LEN * OutData.size(), 1, pFile);
	delete[] FileBuf;
	fclose(pFile);
}
