
#include "pch.h"
#include "CHardwareStatus.h"
//----------------------------------------------------------------------
// 생성자, 확인대상 프로세스 핸들. 미입력시 자기 자신.
//----------------------------------------------------------------------
CHardwareStatus::CHardwareStatus(HANDLE hProcess)
{
	SYSTEM_INFO SystemInfo;
	WCHAR       Buffer[MAX_PATH];
	//------------------------------------------------------------------
	// 프로세스 핸들 입력이 없다면 자기 자신을 대상으로...
	//------------------------------------------------------------------
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		_hProcess = GetCurrentProcess();
	}

	GetProcessName();

	if (_NonPagedPoolQuery == nullptr)
	{
		PdhOpenQuery(NULL, NULL, &_NonPagedPoolQuery);
	}
	if (_ProcessNonPagedPoolQuery == nullptr)
	{
		PdhOpenQuery(NULL, NULL, &_ProcessNonPagedPoolQuery);
	}
	if (_AvailableMemoryQuery == nullptr)
	{
		PdhOpenQuery(NULL, NULL, &_AvailableMemoryQuery);
	}
	if (_EthernetQuery == nullptr)
	{
		PdhOpenQuery(NULL, NULL, &_EthernetQuery);
	}
	//------------------------------------------------------------------
	// 프로세서 개수를 확인한다.
	//
	// 프로세스 (exe) 실행률 계산시 cpu 개수로 나누기를 하여 실제 사용률을 구함.
	//------------------------------------------------------------------
	GetSystemInfo(&SystemInfo);
	_iNumberOfProcessors = SystemInfo.dwNumberOfProcessors;

	LoadEthernetInfo();

	_fProcessorTotal = 0;
	_fProcessorUser = 0;
	_fProcessorKernel = 0;
	_fProcessTotal = 0;
	_fProcessUser = 0;
	_fProcessKernel = 0;
	_ftProcessor_LastKernel.QuadPart = 0;
	_ftProcessor_LastUser.QuadPart = 0;
	_ftProcessor_LastIdle.QuadPart = 0;
	_ftProcess_LastUser.QuadPart = 0;
	_ftProcess_LastKernel.QuadPart = 0;
	_ftProcess_LastTime.QuadPart = 0;

	_iNonPagedPoolBytes = 0;
	_iProcessNonPagedPoolBytes = 0;
	_iAvailableMemoryBytes = 0;
	_iUsedMemoryBytes = 0;

	_dEthernetRecvBytes = 0;
	_dEthernetSendBytes = 0;

	swprintf_s(Buffer, L"\\Process(%s)\\Pool Nonpaged Bytes", _ProcessName);
	PdhAddCounter(_ProcessNonPagedPoolQuery, Buffer, NULL, &_ProcessNonPagedPoolCounter);
	PdhAddCounter(_NonPagedPoolQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &_NonPagedPoolCounter);
	PdhAddCounter(_AvailableMemoryQuery, L"\\Memory\\Available Bytes", NULL, &_AvailableMemoryCounter);


	UpdateHardwareStatus();
}

CHardwareStatus::~CHardwareStatus(void)
{
	if (_NonPagedPoolQuery) PdhCloseQuery(_NonPagedPoolQuery);
	if (_ProcessNonPagedPoolQuery) PdhCloseQuery(_ProcessNonPagedPoolQuery);
	if (_AvailableMemoryQuery) PdhCloseQuery(_AvailableMemoryQuery);
	if (_EthernetQuery) PdhCloseQuery(_EthernetQuery);
	if (_hProcess != GetCurrentProcess()) CloseHandle(_hProcess);
}

void CHardwareStatus::GetProcessName(void)
{
	HANDLE process_handle;
	WCHAR* p;
	int    iLen;

	process_handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	if (process_handle)
	{
		GetModuleFileNameExW(process_handle, 0, _ProcessName, MAX_PATH);
		CloseHandle(process_handle);
	}

	iLen = wcslen(_ProcessName);
	p = _ProcessName + iLen;

	for (int i = iLen - 1; i >= 0; i--)
	{
		if (*p == L'.')
		{
			*p = L'\0';
		}
		if (*p == L'\\')
		{
			break;
		}
		p--;
	}

	p++;

	wcscpy_s(_ProcessName, p);
}

bool CHardwareStatus::LoadEthernetInfo(void)
{

	PDH_STATUS ret;

	int iCnt = 0;
	bool bErr = false;

	WCHAR* szCur = NULL;
	WCHAR* szCounters = NULL;
	WCHAR* szInterfaces = NULL;
	DWORD  dwCounterSize = 0, dwInterfaceSize = 0;
	WCHAR  szQuery[1024] = { 0, };

	PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);
	szCounters = new WCHAR[dwCounterSize];
	szInterfaces = new WCHAR[dwInterfaceSize];


	if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD,
		0) != ERROR_SUCCESS)
	{
		delete[] szCounters;
		delete[] szInterfaces;
		return FALSE;
	}
	iCnt = 0;
	szCur = szInterfaces;

	for (; *szCur != L'\0' && iCnt < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, iCnt++)
	{
		_EthernetStruct[iCnt]._bUse = true;
		_EthernetStruct[iCnt]._szName[0] = L'\0';
		wcscpy_s(_EthernetStruct[iCnt]._szName, szCur);
		szQuery[0] = L'\0';
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);
		ret = PdhAddCounter(_EthernetQuery, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes);
		szQuery[0] = L'\0';
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);
		ret = PdhAddCounter(_EthernetQuery, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes);
	}

	delete[] szCounters;
	delete[] szInterfaces;
	return TRUE;
}


void CHardwareStatus::UpdateHardwareStatus()
{
	//---------------------------------------------------------
	// 프로세서 사용률
	ULARGE_INTEGER Idle;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;

	PDH_STATUS ret;
	PDH_FMT_COUNTERVALUE pdhValue;

	PROCESS_MEMORY_COUNTERS_EX  pmc;

	PDH_FMT_COUNTERVALUE pfc;

	//---------------------------------------------------------
	// 시스템 사용 시간
	//---------------------------------------------------------
	if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false)
	{
		return;
	}
	ULONGLONG KernelDiff = Kernel.QuadPart - _ftProcessor_LastKernel.QuadPart;
	ULONGLONG UserDiff = User.QuadPart - _ftProcessor_LastUser.QuadPart;
	ULONGLONG IdleDiff = Idle.QuadPart - _ftProcessor_LastIdle.QuadPart;
	ULONGLONG Total = KernelDiff + UserDiff;
	ULONGLONG TimeDiff;
	if (Total == 0)
	{
		_fProcessorUser = 0.0f;
		_fProcessorKernel = 0.0f;
		_fProcessorTotal = 0.0f;
	}
	else
	{
		_fProcessorTotal = (float)((double)(Total - IdleDiff) / Total * 100.0f);
		_fProcessorUser = (float)((double)UserDiff / Total * 100.0f);
		_fProcessorKernel = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
	}
	_ftProcessor_LastKernel = Kernel;
	_ftProcessor_LastUser = User;
	_ftProcessor_LastIdle = Idle;


	ULARGE_INTEGER None;
	ULARGE_INTEGER NowTime;

	GetSystemTimeAsFileTime((LPFILETIME)&NowTime);

	GetProcessTimes(_hProcess, (LPFILETIME)&None, (LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);


	TimeDiff = NowTime.QuadPart - _ftProcess_LastTime.QuadPart;
	UserDiff = User.QuadPart - _ftProcess_LastUser.QuadPart;
	KernelDiff = Kernel.QuadPart - _ftProcess_LastKernel.QuadPart;
	Total = KernelDiff + UserDiff;
	_fProcessTotal = (float)(Total / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessKernel = (float)(KernelDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessUser = (float)(UserDiff / (double)_iNumberOfProcessors / (double)TimeDiff * 100.0f);
	_ftProcess_LastTime = NowTime;
	_ftProcess_LastKernel = Kernel;
	_ftProcess_LastUser = User;

	//메모리 사용량 =======================================================================

	PdhCollectQueryData(_NonPagedPoolQuery);
	ret = PdhGetFormattedCounterValue(_NonPagedPoolCounter, PDH_FMT_LARGE, nullptr, &pdhValue);
	if (ret == ERROR_SUCCESS)
	{
		_iNonPagedPoolBytes = pdhValue.largeValue;
	}

	PdhCollectQueryData(_ProcessNonPagedPoolQuery);
	ret = PdhGetFormattedCounterValue(_ProcessNonPagedPoolCounter, PDH_FMT_LARGE, nullptr, &pdhValue);
	if (ret == ERROR_SUCCESS)
	{
		_iProcessNonPagedPoolBytes = pdhValue.largeValue;
	}

	PdhCollectQueryData(_AvailableMemoryQuery);
	ret = PdhGetFormattedCounterValue(_AvailableMemoryCounter, PDH_FMT_LARGE, nullptr, &pdhValue);
	if (ret == ERROR_SUCCESS)
	{
		_iAvailableMemoryBytes = pdhValue.largeValue;
	}

	
	if (GetProcessMemoryInfo(_hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
	{
		_iUsedMemoryBytes = pmc.PrivateUsage;
	}

	//네트워크 사용량 =======================================================================

	_dEthernetRecvBytes = 0;
	_dEthernetSendBytes = 0;

	PdhCollectQueryData(_EthernetQuery);

	for (int iCnt = 0; iCnt < df_PDH_ETHERNET_MAX; iCnt++)
	{
		if (_EthernetStruct[iCnt]._bUse)
		{

			ret = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes,
				PDH_FMT_DOUBLE, NULL, &pfc);

			if (ret == ERROR_SUCCESS)
			{
				_dEthernetRecvBytes += pfc.doubleValue;
			}

			ret = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes,
				PDH_FMT_DOUBLE, NULL, &pfc);

			if (ret == ERROR_SUCCESS)
			{
				_dEthernetSendBytes += pfc.doubleValue;
			}
		}
	}
}

void CHardwareStatus::PrintHardwareStatus(void)
{
	wprintf(L"[CPU Total        ] %4.2f%% (Kernel : %4.2f%% User : %4.2f%%)\n",ProcessorTotal(), ProcessorKernel(), ProcessorUser());
	wprintf(L"[CPU Process      ] %4.2f%% (Kernel : %4.2f%% User : %4.2f%%)\n", ProcessTotal(), ProcessKernel(), ProcessUser());
	wprintf(L"[Memory Total     ] NonPaged : %lld mb / Available : %lld mb \n", NonPagedPoolMB(), AvailableMemoryMB());
	wprintf(L"[Memory Process   ] NonPaged : %lld mb / Used : %lld mb\n", ProcessNonPagedPoolMB(), UsedMemoryMB());
	wprintf(L"[Network(Ethernet)] In : %.2lf kb      / Out : %.2lf kb \n", InNetworkKB(), OutNetworkKB());
}