#pragma once
#include "CLanServer.h"
#include "CMonitoringCSServer.h"
#include "CDBConnector.h"
#include "MonitorProtocol.h"

#define dfMONITORING_SETTING_FILENAME L"MonitoringServer.cnf"
#define dfMONITORING_SERVERNAME       L"MONITOR_SERVER"
#define dfMONITORING_DBNAME           L"MONITOR_DB"

class CMonitoringServer :public CLanServer
{
public:

	struct stServer
	{
		__int64 iSessionID;
		bool    bIsAlive;
		char	chType;

		stServer()
		{
			iSessionID = -1;
			bIsAlive = FALSE;
			chType = -1;
		}
	};

	CMonitoringServer();
	~CMonitoringServer();

	void StartMonitoringServer(const WCHAR* FileName);
	void StopMonitoringServer();

private:

	static unsigned __stdcall DBExecuter(LPVOID parameter)
	{
		CMonitoringServer* pThread = (CMonitoringServer*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->DBThread(nullptr);
		return 0;
	}

	unsigned DBThread(void* lpParam);
	void     DBFunc(void);

	void InitDB(void);
	void InitMonitoringServer(void);

	void OnClientAccept(SESSION_ID iSessionID, short shPort) override;
	void OnClientLeave(SESSION_ID iSessionID) override;

	// wsarecv/send
	void OnRecv(SESSION_ID iSessionID, CPacket* pPacket) override;
	void OnSend(SESSION_ID iSessionID, int sendsize) override {}

	void OnTimeOut(SESSION_ID iSessionID) override
	{
		Disconnect(iSessionID);
	}
	void OnServerClose(void) override
	{
		StopMonitoringServer();
	}
	void OnServerControl(char ch) override {}
	void OnMonitoring(void) override;
	void OnError(int iErrorCode, WCHAR* wch) override {}

	void	 PacketProc(SESSION_ID iSessionID, CPacket* pPacket);
	void	 PacketProcLogin(SESSION_ID iSessionID, CPacket* pPacket);
	void	 PacketProcUpdate(SESSION_ID iSessionID, CPacket* pPacket);
	void     UpdateLog(char chServer, char chType, int iValue);
	CPacket* mpMonitoringSCUpdate(char chServer, char chType, int iValue, int iTimeStamp);
	void     SendMonitoringPacket(char chType, int iValue, int iTimeStamp);
	void     SendServerData(void);

	void	  ServerMapLock(void);
	void	  ServerMapUnLock(void);
	stServer* FindServer(SESSION_ID iSessionID);

	HANDLE				hDBThread;
	CMonitoringCSServer CSServer;
	CDBConnector		DB;
	DWORD				dwDBUpdateTime;
	stDBLog             DBLog[dfMONITORING_MAX_TYPECODE];

	std::unordered_map<SESSION_ID, stServer*> ServerMap;
	CObjectFreeList<stServer>*               ServerPool;
	int									   iMaxServer;
	SRWLOCK ServerLock; //ServerMap Lock

	//LOG
	int iLoginCount;
	int iChatCount;
	int iGameCount;
	int iDBQueryCount;
	
	CHardwareStatus HardwareStatus;
};