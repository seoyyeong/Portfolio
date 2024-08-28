#pragma once

#include "pch.h"
#include "CRoomSession.h"
#include "Protocol.h"
#include "Job.h"


#define dfNETSERVER_MAX_WSA         150
#define dfNETSERVER_MAX_THREADCOUNT 32

#define dfSESSIONINDEX_SHIFT 32
#define dfSESSIONINDEX_MASK  0xFFFFFFFF'00000000

class CRoom;

class CNetRoomServer
{
public:
	friend CRoom;

	CNetRoomServer();
	virtual ~CNetRoomServer();

	DWORD GetTime(void)
	{
		return timeGetTime();
	}

	bool StartServer(const WCHAR* ServerName);
	void StopServer(void);
	void InitServer(const WCHAR* ServerIP, short ServerPort, int Nagle, bool ZeroCopy, int iWorker, int iActive, int MaxSession, DWORD dwLoginTimeOut, DWORD dwReserveDisconnect, bool bCheckTimeOut, bool bReserveDisconnect);

private:

	//�ʱ�ȭ ���� �Լ�
	void InitVariable(void);	//IP, ��Ʈ �� �ʱⰪ �Է�
	void InitSession(void);		//���� �迭 �Ҵ� �� ���� ���� �ʱ�ȭ
	void InitNetwork(void);		//listen ���� �� ��Ʈ��ũ ���� �ʱ�ȭ
	void InitThread(void);		//������ ����

	//������ ���� �Լ�
	static unsigned __stdcall AcceptExecuter(LPVOID parameter)
	{
		CNetRoomServer* pThread = (CNetRoomServer*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->AcceptThread(nullptr);
		return 0;
	}
	static unsigned __stdcall WorkerExecuter(LPVOID parameter)
	{
		CNetRoomServer* pThread = (CNetRoomServer*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->WorkerThread(nullptr);
		return 0;
	}
	static unsigned __stdcall MonitoringExecuter(LPVOID parameter)
	{
		CNetRoomServer* pThread = (CNetRoomServer*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->MonitoringThread(nullptr);
		return 0;
	}

	static unsigned __stdcall CheckDisconnectExecuter(LPVOID parameter)
	{
		CNetRoomServer* pThread = (CNetRoomServer*)parameter;
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
	CRoomSession* SetSession(SOCKET sock, SOCKADDR_IN* SockAddr);
	CRoomSession* FindSession(SESSION_ID iSessionID);

	int IncreaseIOCount(CRoomSession* pSession);

	void DecreaseIOCount(CRoomSession* pSession);

	CRoomSession* AcquireSession(SESSION_ID iSessionID);

	void          ReturnSession(CRoomSession* pSession);

	bool       Release(CRoomSession* pSession);

	//��Ʈ��ũ�� ���� �Լ�
	void	   RecvProc(CRoomSession* pSession, DWORD cbTransferred);
	void       RecvPacket(CRoomSession* pSession);
	bool       CheckPacket(CRoomSession* pSession, CPacket* pPacket);
	void	   RecvPost(CRoomSession* pSession);

	void	   SendProc(CRoomSession* pSession, DWORD cbTransferred);
	bool	   SendPost(CRoomSession* pSession);


	//�ڵ鷯 �Լ���
	virtual void OnClientAccept(SESSION_ID iSessionID, short shPort) = 0;
	virtual void OnClientLeave(SESSION_ID iSessionID) = 0;

	virtual void OnSend(SESSION_ID iSessionID, int iSendSize) = 0;
	virtual void OnTimeOut(SESSION_ID iSessionID) = 0;

	virtual void OnServerStart(void) = 0;
	virtual void OnServerClose(void) = 0;
	virtual void OnServerControl(char ch) {}

	virtual void OnMonitoring(void) = 0;
	virtual void OnError(int iErrorCode, WCHAR* wch) = 0;
	//////////////////////////////

	//��Ʈ��ũ��
	HANDLE		   hIOCP;
	SOCKET		   ListenSocket;
	SOCKADDR_IN    ServerAddr;
	WCHAR          ServerIP[20];
	short          ServerPort;
	char		   chHeaderCode;
	char		   chHeaderKey;
	int			   iNagle;
	bool		   bZeroCopySend;

	bool           bCheckTimeOut;
	bool           bReserveDisconnect;

	DWORD          dwLoginTimeOut;
	DWORD          dwReserveDisconnect;

	//������ ������
	HANDLE		   ThreadArr[dfNETSERVER_MAX_THREADCOUNT];	//������ �迭
	HANDLE		   hExitEvent;					//������ ���� �̺�Ʈ
	int			   iWorkerCnt;			        //IOCP ��Ŀ������ ����
	int			   iActiveWorkerCnt;			//IOCP ��Ŀ������ ����
	int			   iThreadCnt;		    //�� ������ ����


	//���� ������ 
	SESSION_ID		           iSessionID;
	CRoomSession*			   SessionArr;	       //���� �迭
	CLockFreeStack<int>*       SessionOpenList;    //���� �迭�� �� ������ ��� �ڷᱸ��

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
	bool		 SetTimeOut(SESSION_ID iSessionID, DWORD dwTime);
	bool		 Disconnect(SESSION_ID iSessionID);
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

	CTlsPool<stJob> JobPool;

	bool   bRunning;
};
