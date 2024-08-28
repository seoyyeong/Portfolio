#pragma once

#include "pch.h"
#include "CSession.h"
#include "Job.h"
#include "Protocol.h"


#define MAX_WSA         256
#define MAX_THREADCOUNT 128

#define dfSESSIONINDEX_SHIFT 32
#define dfSESSIONINDEX_MASK  0xFFFFFFFF'00000000


class CLanClient
{
public:

	CLanClient();
	virtual ~CLanClient();


	inline DWORD GetTime(void)
	{
		return timeGetTime();
	}

	bool StartClient(void);
	void StopClient(void);
	inline bool GetClientStatus(void)
	{
		if (Session.iSessionID == -1)
		{
			return FALSE;
		}
		return TRUE;
	}
	void InitClient(const WCHAR* ServerIP, short ServerPort, int iWorker, int iActive);

private:

	//초기화 관련 함수
	void InitThread(void);		//스레드 시작

	//스레드 관련 함수

	static unsigned __stdcall WorkerExecuter(LPVOID parameter)
	{
		CLanClient* pThread = (CLanClient*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->WorkerThread(nullptr);
		return 0;
	}
	static unsigned __stdcall MonitoringExecuter(LPVOID parameter)
	{
		CLanClient* pThread = (CLanClient*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->MonitoringThread(nullptr);
		return 0;
	}

	bool             ConnectFunc(void);
	unsigned         WorkerThread(void* lpParam);
	virtual unsigned MonitoringThread(void* lpParam);

	//세션 관련 함수
	bool        SetSession(void);
	inline int  IncreaseIOCount(void)
	{
		return InterlockedIncrement((long*)&Session.iIOCount);
	}

	inline void DecreaseIOCount(void)
	{
		int ret;

		ret = InterlockedDecrement((long*)&Session.iIOCount);

		if (ret == 0)
		{
			Release();
			return;
		}
	}

	bool       Release(void);

	//네트워크부 관련 함수
	void	   RecvProc(DWORD cbTransferred);
	bool       RecvPacket(CPacket* pPacket);
	void	   RecvPost(void);

	void	   SendProc(DWORD cbTransferred);
	bool	   SendPost(void);


	//핸들러 함수들
	virtual bool OnConnectionRequest(WCHAR* IP, short shPort) = 0;
	virtual void OnServerConnect(short shPort) = 0;
	virtual void OnServerLeave(void) = 0;

	virtual void OnRecv(CPacket* pPacket) = 0;
	virtual void OnSend(int iSendSize) = 0;

	virtual void OnError(int iErrorCode, WCHAR* wch) = 0;
	//////////////////////////////

	//네트워크부
	HANDLE		   hIOCP;

	SOCKADDR_IN    ServerAddr;
	WCHAR          ServerIP[20];

	short          ServerPort;

	//스레드 관리부
	HANDLE		   ThreadArr[MAX_THREADCOUNT];	//스레드 배열
	HANDLE		   hExitEvent;					//스레드 종료 이벤트
	int			   iWorkerCnt;			        //IOCP 워커스레드 개수
	int			   iActiveWorkerCnt;			//IOCP 워커스레드 개수
	int			   iThreadCnt;				    //총 스레드 개수

	//세션 관리부 
	CSession       Session;
	SESSION_ID     iSessionIdx;

	DWORD		   dwLoginTimeOut;
	DWORD		   dwReserveDisconnect;


	int iRecvTPS;
	int iSendTPS;
	int iRecvBytesTPS;
	int iSendBytesTPS;
	int iRecvMsg;
	int iSendMsg;


protected:

	virtual void   MonitoringFunc(void);
	bool		   ConnectToServer(void);
	void		   PostConnect(void);
	bool		   Disconnect(void);
	bool		   SendPacket(CPacket* pPacket, bool bReserveRelease = FALSE);

	inline int GetRecvTPS(void)
	{
		return InterlockedExchange((long*)&iRecvTPS, 0);
	}
	inline int GetSendTPS(void)
	{
		return InterlockedExchange((long*)&iSendTPS, 0);
	}
	inline int GetRecvBytesTPS(void)
	{
		return InterlockedExchange((long*)&iRecvBytesTPS, 0);
	}
	inline int GetSendBytesTPS(void)
	{
		return InterlockedExchange((long*)&iSendBytesTPS, 0);
	}
	inline int GetRecvMsgTPS(void)
	{
		return InterlockedExchange((long*)&iRecvMsg, 0);
	}
	inline int GetSendMsgTPS(void)
	{
		return InterlockedExchange((long*)&iSendMsg, 0);
	}

	bool		   bRunning;


	//로그 관리부
	CHardwareStatus HardwareStatus;
	int				iLogLevel;


	//설정파일 : 상속받는 클래스에서 불러와야 합니다.
	CParser Parse;

};
