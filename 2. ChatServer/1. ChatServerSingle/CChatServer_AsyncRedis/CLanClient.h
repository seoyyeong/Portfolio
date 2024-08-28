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

	//�ʱ�ȭ ���� �Լ�
	void InitThread(void);		//������ ����

	//������ ���� �Լ�

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

	//���� ���� �Լ�
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

	//��Ʈ��ũ�� ���� �Լ�
	void	   RecvProc(DWORD cbTransferred);
	bool       RecvPacket(CPacket* pPacket);
	void	   RecvPost(void);

	void	   SendProc(DWORD cbTransferred);
	bool	   SendPost(void);


	//�ڵ鷯 �Լ���
	virtual bool OnConnectionRequest(WCHAR* IP, short shPort) = 0;
	virtual void OnServerConnect(short shPort) = 0;
	virtual void OnServerLeave(void) = 0;

	virtual void OnRecv(CPacket* pPacket) = 0;
	virtual void OnSend(int iSendSize) = 0;

	virtual void OnError(int iErrorCode, WCHAR* wch) = 0;
	//////////////////////////////

	//��Ʈ��ũ��
	HANDLE		   hIOCP;

	SOCKADDR_IN    ServerAddr;
	WCHAR          ServerIP[20];

	short          ServerPort;

	//������ ������
	HANDLE		   ThreadArr[MAX_THREADCOUNT];	//������ �迭
	HANDLE		   hExitEvent;					//������ ���� �̺�Ʈ
	int			   iWorkerCnt;			        //IOCP ��Ŀ������ ����
	int			   iActiveWorkerCnt;			//IOCP ��Ŀ������ ����
	int			   iThreadCnt;				    //�� ������ ����

	//���� ������ 
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


	//�α� ������
	CHardwareStatus HardwareStatus;
	int				iLogLevel;


	//�������� : ��ӹ޴� Ŭ�������� �ҷ��;� �մϴ�.
	CParser Parse;

};
