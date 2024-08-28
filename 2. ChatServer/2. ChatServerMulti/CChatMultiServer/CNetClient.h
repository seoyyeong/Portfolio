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

	struct NetLog  //����Ÿ�� �α� ��ü
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

	//�ʱ�ȭ ���� �Լ�
	void InitVariable(const WCHAR* ClientName);	//IP, ��Ʈ �� �ʱⰪ �Է�
	void InitNetwork(void);		//listen ���� �� ��Ʈ��ũ ���� �ʱ�ȭ
	void InitThread(void);		//������ ����

	//������ ���� �Լ�

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

	//���� ���� �Լ�
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

	//��Ʈ��ũ�� ���� �Լ�
	void	   RecvProc(DWORD cbTransferred);
	bool       RecvPacket(CPacket* pPacket);
	void	   RecvPost(void);

	void	   SendProc(DWORD cbTransferred);
	bool	   SendPost(void);


	//�ڵ鷯 �Լ���
	virtual bool OnConnectionRequest(WCHAR* IP, short shPort) = 0;
	virtual void OnClientConnect(short shPort) = 0;
	virtual void OnClientLeave(void) = 0;

	virtual void OnRecv(CPacket* pPacket) = 0;
	virtual void OnSend(int iSendSize) = 0;

	virtual void OnError(int iErrorCode, WCHAR* wch) = 0;
	//////////////////////////////

	//��Ʈ��ũ��
	SOCKET		   ListenSocket;
	long		   ServerIP;
	__int64		   iSessionID;
	unsigned short ServerPort;
	SOCKADDR_IN    ServerAddr;

	//���� ������ 
	stSession      Session;


protected:
	virtual void   MonitoringFunc(void);
	bool		   Disconnect(void);
	bool		   SendPacket(CPacket* pPacket, bool bReserveRelease = FALSE);

	bool		   bRunning;

	//��Ʈ��ũ��
	SOCKET		   ListenSocket;
	SOCKADDR_IN    ServerAddr;
	WCHAR          ServerIP[20];
	short          ServerPort;
	char		   chHeaderCode;
	char		   chHeaderKey;

	//���� ������ 
	__int64		               iSessionID;
	stSession*				   SessionArr;	       //���� �迭
	CLockFreeStack<int>*	   SessionOpenList;    //���� �迭�� �� ������ ��� �ڷᱸ��

	std::queue<stSession*>     DisconnectQueue;    //release reserve queue
	SRWLOCK                    DisconnectQueueLock;

	__int64					   iMaxSession;	    //�ִ� ����
	__int64					   iSessionCount;
	DWORD					   dwTimeOut;
	DWORD					   dwReserveDisconnect;

protected:
	virtual void   MonitoringFunc(void);
	bool		   Disconnect(void);
	bool		   SendPacket(CPacket* pPacket, bool bReserveRelease = FALSE);

	HANDLE		   hIOCP;

	//������ ������
	HANDLE		   ThreadArr[MAX_THREADCOUNT];	//������ �迭
	HANDLE		   hExitEvent;					//������ ���� �̺�Ʈ
	int			   iWorkerCnt;			        //IOCP ��Ŀ������ ����
	int			   iActiveWorkerCnt;			//IOCP ��Ŀ������ ����
	int			   iThreadCnt;		    //�� ������ ����

	//�α� ������
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

	//�������� : ��ӹ޴� Ŭ�������� �ҷ��;� �մϴ�.
	CParser Parse;

};
