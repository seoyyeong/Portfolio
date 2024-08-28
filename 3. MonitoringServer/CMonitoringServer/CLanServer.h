#pragma once

#include "pch.h"
#include "Job.h"
#include "Protocol.h"
#include "CSession.h"


#define dfLANSERVER_MAX_WSA         150
#define dfLANSERVER_MAX_THREADCOUNT 32

#define dfSESSIONINDEX_SHIFT 32
#define dfSESSIONINDEX_MASK  0xFFFFFFFF'00000000

class CLanServer
{
public:


	CLanServer();
	virtual ~CLanServer();

	DWORD GetTime(void)
	{
		return timeGetTime();
	}

	bool StartServer(void);
	void StopServer(void);

private:

	//�ʱ�ȭ ���� �Լ�
	void InitVariable(void);	//IP, ��Ʈ �� �ʱⰪ �Է�
	void InitSession(void);		//���� �迭 �Ҵ� �� ���� ���� �ʱ�ȭ
	void InitNetwork(void);		//listen ���� �� ��Ʈ��ũ ���� �ʱ�ȭ
	void InitThread(void);		//������ ����

	//������ ���� �Լ�
	static unsigned __stdcall AcceptExecuter(LPVOID parameter)
	{
		CLanServer* pThread = (CLanServer*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->AcceptThread(nullptr);
		return 0;
	}
	static unsigned __stdcall WorkerExecuter(LPVOID parameter)
	{
		CLanServer* pThread = (CLanServer*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->WorkerThread(nullptr);
		return 0;
	}
	static unsigned __stdcall MonitoringExecuter(LPVOID parameter)
	{
		CLanServer* pThread = (CLanServer*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->MonitoringThread(nullptr);
		return 0;
	}

	static unsigned __stdcall CheckDisconnectExecuter(LPVOID parameter)
	{
		CLanServer* pThread = (CLanServer*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->CheckDisconnectThread(nullptr);
		return 0;
	}

	unsigned         AcceptThread(void* lpParam);
	unsigned         WorkerThread(void* lpParam);
	virtual unsigned MonitoringThread(void* lpParam);
	unsigned         CheckDisconnectThread(void* lpParam);

	//���� ���� �Լ�
	CSession* SetSession(SOCKET sock, SOCKADDR_IN* SockAddr);
	CSession* FindSession(SESSION_ID iSessionID)
	{
		if (iSessionID == -1)
		{
			return nullptr;
		}

		int idx = iSessionID >> dfSESSIONINDEX_SHIFT;

		if (SessionArr[idx].iSessionID != iSessionID)
		{
			return nullptr;
		}

		return &SessionArr[idx];
	}

	int  IncreaseIOCount(CSession* pSession)
	{
		return InterlockedIncrement((long*)&pSession->iIOCount);
	}

	void DecreaseIOCount(CSession* pSession)
	{
		int ret;

		ret = InterlockedDecrement((long*)&pSession->iIOCount);

		if (ret == 0)
		{
			Release(pSession);
			return;
		}

	}

	bool       Release(CSession* pSession);

	//��Ʈ��ũ�� ���� �Լ�
	void	   RecvProc(CSession* pSession, DWORD cbTransferred);
	void       RecvPacket(CSession* pSession);
	bool       CheckPacket(CSession* pSession, CPacket* pPacket);
	void	   RecvPost(CSession* pSession);

	void	   SendProc(CSession* pSession, DWORD cbTransferred);
	bool	   SendPost(CSession* pSession);


	//�ڵ鷯 �Լ���
	virtual void OnClientAccept(SESSION_ID iSessionID, short shPort) = 0;
	virtual void OnClientLeave(SESSION_ID iSessionID) = 0;

	virtual void OnRecv(SESSION_ID iSessionID, CPacket* pPacket) = 0;
	virtual void OnSend(SESSION_ID iSessionID, int iSendSize) = 0;
	virtual void OnTimeOut(SESSION_ID iSessionID) = 0;
	virtual void OnMonitoring(void) = 0;

	virtual void OnServerClose(void) = 0;
	virtual void OnServerControl(char ch) {}

	virtual void OnError(int iErrorCode, WCHAR* wch) = 0;
	//////////////////////////////

	//��Ʈ��ũ��
	HANDLE		   hIOCP;
	SOCKET		   ListenSocket;
	SOCKADDR_IN    ServerAddr;
	WCHAR          ServerIP[20];
	short          ServerPort;
	int			   iNagle;
	bool		   bZeroCopySend;

	bool           bCheckTimeOut;
	bool           bReserveDisconnect;

	DWORD          dwLoginTimeOut;
	DWORD          dwReserveDisconnect;

	//������ ������
	HANDLE		   ThreadArr[dfLANSERVER_MAX_THREADCOUNT];	//������ �迭
	HANDLE		   hExitEvent;					//������ ���� �̺�Ʈ
	int			   iWorkerCnt;			        //IOCP ��Ŀ������ ����
	int			   iActiveWorkerCnt;			//IOCP ��Ŀ������ ����
	int			   iThreadCnt;		    //�� ������ ����


	//���� ������ 
	SESSION_ID iSessionID;
	CSession* SessionArr;	       //���� �迭
	CLockFreeStack<int>* SessionOpenList;    //���� �迭�� �� ������ ��� �ڷᱸ��

	CLockFreeQueue<SESSION_ID>    DisconnectQueue;    //release reserve queue

	int					   iMaxSession;	    //�ִ� ����
	int					   iSessionCount;

	//����͸� ����

	void RefreshTPS(void)
	{
		InterlockedExchange((long*)&iRecvTPS, 0);
		InterlockedExchange((long*)&iSendTPS, 0);
		InterlockedExchange((long*)&iRecvBytesTPS, 0);
		InterlockedExchange((long*)&iSendBytesTPS, 0);
		InterlockedExchange((long*)&iRecvMsg, 0);
		InterlockedExchange((long*)&iSendMsg, 0);
		InterlockedExchange((long*)&iAcceptTPS, 0);
		InterlockedExchange((long*)&iReleaseTPS, 0);
	}

	int iRecvTPS;
	int iSendTPS;
	int iRecvBytesTPS;
	int iSendBytesTPS;
	int iRecvMsg;
	int iSendMsg;
	int	iAcceptTPS;
	int iReleaseTPS;
	int iTotalAccept;
	int iTotalRelease;

protected:

	void         InitServer(const WCHAR* ServerIP, short ServerPort, int Nagle, bool ZeroCopy, int iWorker, int iActive, int MaxSession, DWORD dwLoginTimeOut, DWORD dwReserveDisconnect, bool bCheckTimeOut, bool bReserveDisconnect);
	bool		 SetTimeOut(SESSION_ID iSessionID, DWORD dwTime);
	bool		 Disconnect(SESSION_ID iSessionID, short shType = 0);
	bool		 SendPacket(SESSION_ID iSessionID, CPacket* pPacket, bool bReserveRelease = FALSE);
	bool		 SendPackets(SESSION_ID iSessionID, CPacket** pPacketArr, int iCount, bool bReserveRelease = FALSE);
	bool		 SendPacketInstant(SESSION_ID iSessionID, CPacket* pPacket, bool bReserveRelease = FALSE);

	//TPS Getter

	int GetSessionCount(void)
	{
		return InterlockedOr((long*)&iSessionCount, 0);
	}
	int GetRecvTPS(void)
	{
		return InterlockedOr((long*)&iRecvTPS, 0);
	}
	int GetSendTPS(void)
	{
		return InterlockedOr((long*)&iSendTPS, 0);
	}
	int GetRecvBytesTPS(void)
	{
		return InterlockedOr((long*)&iRecvBytesTPS, 0);
	}
	int GetSendBytesTPS(void)
	{
		return InterlockedOr((long*)&iSendBytesTPS, 0);
	}
	int GetRecvMsgTPS(void)
	{
		return InterlockedOr((long*)&iRecvMsg, 0);
	}
	int GetSendMsgTPS(void)
	{
		return InterlockedOr((long*)&iSendMsg, 0);
	}
	int GetAcceptTPS(void)
	{
		return InterlockedOr((long*)&iAcceptTPS, 0);
	}
	int GetReleaseTPS(void)
	{
		return InterlockedOr((long*)&iReleaseTPS, 0);
	}
	int GetTotalAccept(void)
	{
		return InterlockedOr((long*)&iTotalAccept, 0);
	}
	int GetTotalRelease(void)
	{
		return InterlockedOr((long*)&iTotalRelease, 0);
	}

	//��������
	CParser Parse;

	bool   bRunning;
};
