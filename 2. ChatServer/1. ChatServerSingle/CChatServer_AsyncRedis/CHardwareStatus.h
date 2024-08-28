#pragma once

#include "pch.h"

#define df_PDH_ETHERNET_MAX 8

#ifndef __CPU_USAGE_H__
#define __CPU_USAGE_H__


class CHardwareStatus
{
public:
	struct st_ETHERNET
	{
		bool _bUse;
		WCHAR _szName[128];
		PDH_HCOUNTER _pdh_Counter_Network_RecvBytes;
		PDH_HCOUNTER _pdh_Counter_Network_SendBytes;
	};
	//----------------------------------------------------------------------
	// 생성자, 확인대상 프로세스 핸들. 미입력시 자기 자신.
	//----------------------------------------------------------------------
	CHardwareStatus(HANDLE hProcess = INVALID_HANDLE_VALUE);
	~CHardwareStatus(void);

	void UpdateHardwareStatus(void);
	void PrintHardwareStatus(void);

	__inline float ProcessorTotal(void) { return _fProcessorTotal; }
	__inline float ProcessorUser(void) { return _fProcessorUser; }
	__inline float ProcessorKernel(void) { return _fProcessorKernel; }
	__inline float ProcessTotal(void) { return _fProcessTotal; }
	__inline float ProcessUser(void) { return _fProcessUser; }
	__inline float ProcessKernel(void) { return _fProcessKernel; }

	__inline __int64 NonPagedPoolB (void) { return _iNonPagedPoolBytes; }
	__inline __int64 NonPagedPoolKB(void) { return _iNonPagedPoolBytes / 1'024; }
	__inline __int64 NonPagedPoolMB(void) { return _iNonPagedPoolBytes / 1'048'576; }

	__inline __int64 ProcessNonPagedPoolB(void) { return _iProcessNonPagedPoolBytes; }
	__inline __int64 ProcessNonPagedPoolKB(void) { return _iProcessNonPagedPoolBytes / 1'024; }
	__inline __int64 ProcessNonPagedPoolMB(void) { return _iProcessNonPagedPoolBytes / 1'048'576; }

	__inline __int64 AvailableMemoryB(void) { return _iAvailableMemoryBytes; }
	__inline __int64 AvailableMemoryKB(void) { return _iAvailableMemoryBytes / 1'024; }
	__inline __int64 AvailableMemoryMB(void) { return _iAvailableMemoryBytes / 1'048'576; }

	__inline __int64 UsedMemoryB(void) { return _iUsedMemoryBytes; }
	__inline __int64 UsedMemoryKB(void) { return _iUsedMemoryBytes / 1'024; }
	__inline __int64 UsedMemoryMB(void) { return _iUsedMemoryBytes / 1'048'576; }

	__inline double  InNetworkB(void) { return (_dEthernetRecvBytes); }
	__inline double  InNetworkKB(void) { return _dEthernetRecvBytes / 1'024; }
	__inline double  InNetworkMB(void) { return _dEthernetRecvBytes / 1'048'576; }

	__inline double  OutNetworkB(void) { return _dEthernetSendBytes; }
	__inline double  OutNetworkKB(void) { return _dEthernetSendBytes / 1'024; }
	__inline double  OutNetworkMB(void) { return _dEthernetSendBytes / 1'048'576; }

private:
	bool LoadEthernetInfo(void);
	void GetProcessName(void);

	HANDLE _hProcess;
	WCHAR  _ProcessName[MAX_PATH];
	int    _iNumberOfProcessors;

	ULARGE_INTEGER _ftProcessor_LastKernel;
	ULARGE_INTEGER _ftProcessor_LastUser;
	ULARGE_INTEGER _ftProcessor_LastIdle;
	ULARGE_INTEGER _ftProcess_LastKernel;
	ULARGE_INTEGER _ftProcess_LastUser;
	ULARGE_INTEGER _ftProcess_LastTime;

	PDH_HQUERY   _NonPagedPoolQuery;
	PDH_HCOUNTER _NonPagedPoolCounter;

	PDH_HQUERY   _ProcessNonPagedPoolQuery;
	PDH_HCOUNTER _ProcessNonPagedPoolCounter;

	PDH_HQUERY   _AvailableMemoryQuery;
	PDH_HCOUNTER _AvailableMemoryCounter;

	PDH_HQUERY   _EthernetQuery;
	st_ETHERNET  _EthernetStruct[df_PDH_ETHERNET_MAX]; // 랜카드 별 PDH 정보

	float _fProcessorTotal;
	float _fProcessorUser;
	float _fProcessorKernel;
	float _fProcessTotal;
	float _fProcessUser;
	float _fProcessKernel;

	unsigned __int64 _iNonPagedPoolBytes;
	unsigned __int64 _iProcessNonPagedPoolBytes;
	unsigned __int64 _iAvailableMemoryBytes;
	unsigned __int64 _iUsedMemoryBytes;

	double _dEthernetRecvBytes; // 총 Recv Bytes 모든 이더넷의 Recv 수치 합산
	double _dEthernetSendBytes; // 총 Send Bytes 모든 이더넷의 Send 수치 합산

};
#endif
