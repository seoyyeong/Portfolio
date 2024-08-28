#pragma once

#include "stdafx.h"

#include "CParser.h"
#include "RingBuffer.h"
#include "ProtocolBuffer.h"
#include "CTlsPool.h"
#include "CCrashDump.h"
#include "CLockFreeQueue.h"
#include "CLockFreeStack.h"
#include "CLog.h"
#include "CTlsProfile.h"
#include "Job.h"
#include "Protocol.h"


#define MAX_WSA         256
#define MAX_THREADCOUNT 128

#define dfSESSIONINDEX_SHIFT 32
#define dfSESSIONINDEX_MASK  0xFFFFFFFF'00000000


class CNetClient
{
public:

	enum OVERLAPPEDTYPE
	{
		SEND = 0x10, RECV
	};

	struct stSession;

	struct stOverlapped
	{
		OVERLAPPED stOverlapped;
		stSession* pSession;
		int		   iType;
	};

	struct stSession
	{
		__int64 iSessionID; //-1 : empty Session
		__int64	iSessionidx;

		int  iIOCount;
		bool bSendFlag;
		bool bIsAlive;

		SOCKET			sock;
		unsigned short  shPort;
		u_long          lIP;

		stOverlapped	          stOverRecv;
		RingBuffer		          RecvQ;
		stOverlapped	          stOverSend;
		CLockFreeQueue<CPacket*>  SendQ;

		CPacket*  SendPacketArr[MAX_WSA];
		int       iSendPacket;

		DWORD dwLastSend;
		DWORD dwLastRecv;
		DWORD dwTimeOut;
		DWORD dwReservedRelease;


		stSession()
		{
			iSessionID = -1;

			iIOCount = 0x80000001;
			bSendFlag = FALSE;
			bIsAlive = FALSE;
			sock = INVALID_SOCKET;

			stOverRecv.pSession = this;
			stOverRecv.iType = RECV;
			stOverSend.pSession = this;
			stOverSend.iType = SEND;

			memset(&stOverRecv.stOverlapped, 0, sizeof(OVERLAPPED));
			memset(&stOverSend.stOverlapped, 0, sizeof(OVERLAPPED));
		}

	};

	struct NetLog  //러닝타임 로그 객체
	{
		__int64 iThreadID = 0;
		__int64 iRecv = 0;
		__int64 iSend = 0;
		__int64 iRecvBytes = 0;
		__int64 iSendBytes = 0;
		__int64 iRecvTPS = 0;
		__int64 iSendTPS = 0;
		__int64 iRecvBytesTPS = 0;
		__int64 iSendBytesTPS = 0;
	};

	CNetClient();
	virtual ~CNetClient();


	__inline DWORD GetTime(void)
	{
		return timeGetTime();
	}

	bool StartClient(const WCHAR* ClientName);
	void StopClient(void);

private:

	//초기화 관련 함수
	void InitVariable(const WCHAR* ClientName);	//IP, 포트 등 초기값 입력
	void InitNetwork(void);		//listen 소켓 등 네트워크 관련 초기화
	void InitThread(void);		//스레드 시작

	//스레드 관련 함수

	static unsigned __stdcall WorkerExecuter(LPVOID parameter)
	{
		CNetClient* pThread = (CNetClient*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->WorkerThread(nullptr);
		return 0;
	}
	static unsigned __stdcall MonitoringExecuter(LPVOID parameter)
	{
		CNetClient* pThread = (CNetClient*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->MonitoringThread(nullptr);
		return 0;
	}

	bool             ConnectFunc(void);
	unsigned         WorkerThread(void* lpParam);
	virtual unsigned MonitoringThread(void* lpParam);

	//세션 관련 함수
	void        SetSession(void);
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

		if (ret < 0)
		{
			CRASH();
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
	virtual void OnClientConnect(short shPort) = 0;
	virtual void OnClientLeave(void) = 0;

	virtual void OnRecv(CPacket* pPacket) = 0;
	virtual void OnSend(int iSendSize) = 0;

	virtual void OnError(int iErrorCode, WCHAR* wch) = 0;
	//////////////////////////////

	//네트워크부
	SOCKET		   ListenSocket;
	long		   ServerIP;
	__int64		   iSessionID;
	unsigned short ServerPort;
	SOCKADDR_IN    ServerAddr;

	//세션 관리부 
	stSession      Session;


protected:
	virtual void   MonitoringFunc(void);
	bool		   Disconnect(void);
	bool		   SendPacket(CPacket* pPacket, bool bReserveRelease = FALSE);

	bool		   bRunning;

	//네트워크부
	SOCKET		   ListenSocket;
	SOCKADDR_IN    ServerAddr;
	WCHAR          ServerIP[20];
	short          ServerPort;
	char		   chHeaderCode;
	char		   chHeaderKey;

	//세션 관리부 
	__int64		               iSessionID;
	stSession*				   SessionArr;	       //세션 배열
	CLockFreeStack<int>*	   SessionOpenList;    //세션 배열의 빈 공간을 담는 자료구조

	std::queue<stSession*>     DisconnectQueue;    //release reserve queue
	SRWLOCK                    DisconnectQueueLock;

	__int64					   iMaxSession;	    //최대 세션
	__int64					   iSessionCount;
	DWORD					   dwTimeOut;
	DWORD					   dwReserveDisconnect;

protected:
	virtual void   MonitoringFunc(void);
	bool		   Disconnect(void);
	bool		   SendPacket(CPacket* pPacket, bool bReserveRelease = FALSE);

	HANDLE		   hIOCP;

	//스레드 관리부
	HANDLE		   ThreadArr[MAX_THREADCOUNT];	//스레드 배열
	HANDLE		   hExitEvent;					//스레드 종료 이벤트
	int			   iWorkerCnt;			        //IOCP 워커스레드 개수
	int			   iActiveWorkerCnt;			//IOCP 워커스레드 개수
	int			   iThreadCnt;		    //총 스레드 개수

	//로그 관리부
	WCHAR     LogLevel[10];

	__int64 iRecvCall;
	__int64 iSendCall;
	__int64 iRecvBytes;
	__int64 iSendBytes;
	__int64 iRecvTPS;
	__int64 iSendTPS;
	__int64 iRecvBytesTPS;
	__int64 iSendBytesTPS;
	__int64 iSendMsg;

	//설정파일 : 상속받는 클래스에서 불러와야 합니다.
	CParser Parse;

};
