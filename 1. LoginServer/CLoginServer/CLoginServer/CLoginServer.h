#pragma once

#include "CNetServer.h"
#include "CMonitoringClient.h"
#include "CDBConnectorTls.h"
#include "CRedisTls.h"
#include "MonitorProtocol.h"
#include "CommonProtocol.h"
#define dfLOGIN_SETTING_FILENAME L"LoginServer.cnf"
#define dfLOGIN_SERVERNAME L"LOGIN_SERVER"

// TLSĿ�ؼ� ��뿩��
#define _LOGIN_CONNECTIONPOOL

class CLoginServer :public CNetServer
{
public:

	CLoginServer();
	~CLoginServer();

	void StartLoginServer(const WCHAR* FileName = dfLOGIN_SETTING_FILENAME);
	void StopLoginServer(void);

private:

	//�ʱ�ȭ(pool, ��Ʈ ��)
	void InitLoginVariable(void);

	//////////////////////////////////////�ڵ鷯 : CNetClient ���������Լ� ������
	void OnClientAccept(SESSION_ID iSessionID, short shPort) override {}
	void OnClientLeave(SESSION_ID iSessionID) override {}

	// wsarecv/send
	void OnRecv(SESSION_ID iSessionID, CPacket* pPacket) override;
	void OnSend(SESSION_ID iSessionID, int sendsize) override {}

	void OnTimeOut(SESSION_ID iSessionID) override
	{
		Disconnect(iSessionID);
	}
	void OnServerClose(void) override
	{
		StopLoginServer();
	}
	void OnServerControl(char ch) override
	{
		if (ch == 'c' || ch == 'C')
		{
			if (MonitorClient.GetClientStatus() == FALSE)
			{
				MonitorClient.TryConnect();
			}
		}
	}
	void OnMonitoring(void) override;
	void OnError(int iErrorCode, WCHAR* wch) override {}

	void     PacketProc(SESSION_ID iSessionID, CPacket* pPacket);
	char     CheckAccountValid(__int64 iAccountNo, char* pSessionKey);
	bool     InsertToken(__int64 iAccountNo, char* pSessionKey);
	CPacket* mpLoginResponse(__int64 iAccountNo, char chTokenCheck);

	//DB
	void InitDB(void);

#ifdef _LOGIN_CONNECTIONPOOL
	CRedisTls		  RedisClient;
	CDBConnectorTls   DBConnector;
#else
	cpp_redis::client RedisClient;
	SRWLOCK           RedisLock;
	CDBConnector      DBConnector;
#endif
	char              DBName[20];

	WCHAR	GameServerIP[20];	// ���Ӵ�� ����,ä�� ���� ����
	short	GameServerPort;
	WCHAR	ChatServerIP[20];
	short	ChatServerPort;

	//����͸� �׸� ����

	CMonitoringClient MonitorClient;
	CHardwareStatus HardwareStatus;
	int iLoginTPS;
	
};