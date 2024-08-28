#pragma once
#include "CNetServer.h"
#include "MonitorProtocol.h"

#define dfMONITORING_SETTING_FILENAME L"MonitoringServer.cnf"
#define dfMONITERING_CS_SERVERNAME    L"MONITOR_CS_SERVER"
#define dfMONITORING_CS_KEYSIZE       32

class CMonitoringCSServer :public CNetServer
{
public:
	struct stClient
	{
		SESSION_ID iSessionID;
		bool    bIsAlive;

		stClient()
		{
			iSessionID = -1;
			bIsAlive = FALSE;
		}
	};

	friend class CMonitoringServer;

	CMonitoringCSServer();
	~CMonitoringCSServer();
	void StartMonitoringCSServer(const WCHAR* pFileName = dfMONITORING_SETTING_FILENAME);
	void StopMonitoringCSServer(void);

	int GetSendTPS(void)
	{
		return iSendTPS;
	}
	int GetSendBytesTPS(void)
	{
		return iSendBytesTPS;
	}

private:
	void InitMonitoringCSServer(void);

	//////////////////////////////////////핸들러 : CNetClient 순수가상함수 재정의
	void OnClientAccept(SESSION_ID iSessionID, short shPort) override;
	void OnClientLeave(SESSION_ID iSessionID) override;

	// wsarecv/send
	void OnRecv(SESSION_ID iSessionID, CPacket* pPacket) override;
	void OnSend(SESSION_ID iSessionID, int sendsize) override {}
	unsigned MonitoringThread(void* lpParam) override { return 0; }

	void OnTimeOut(SESSION_ID iSessionID) override
	{
		Disconnect(iSessionID);
	}
	void OnServerClose(void) override
	{
		StopMonitoringCSServer();
	}
	void OnServerControl(char ch) override {}
	void OnMonitoring(void) override {}
	void OnError(int iErrorCode, WCHAR* wch) override {}

	void     PacketProc(SESSION_ID iSessionID, CPacket* pPacket);
	void     PacketProcCSLogin(SESSION_ID iSessionID, CPacket* pPacket);
	CPacket* mpMonitoringCSLogin(char chStatus);

	void SendPacketToAllClients(CPacket* pPacket);
	
	void ClientMapLock(void);
	void ClientMapUnLock(void);
	stClient* FindClient(SESSION_ID iSessionID);

	void RefreshTPS(void)
	{
		iSendTPS = 0;
		iSendBytesTPS = 0;
	}


	//세션
	std::unordered_map<SESSION_ID, stClient*> ClientMap;
	CObjectFreeList<stClient>*                ClientPool;


	int     iMaxClient;
	SRWLOCK ClientLock;
	char    SessionKey[dfMONITORING_CS_KEYSIZE + 1];

	int     iSendTPS;
	int     iSendBytesTPS;
	
	DWORD   dwCSTimeOut;

};