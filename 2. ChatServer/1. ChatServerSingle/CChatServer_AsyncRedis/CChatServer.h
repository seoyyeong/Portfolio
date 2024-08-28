#pragma once

#include "CNetServer.h"
#include "CommonProtocol.h"
#include "CMonitoringClient.h"
#include "CPlayer.h"
#include "CChatAsyncRedis.h"

#define dfCHAT_SETTING_FILENAME L"ChatServer.cnf"
#define dfCHATSERVER_NAME       L"CHAT_SERVER"
#define dfCHATSERVER_MAX_THREADCOUNT 32

using PLAYER_MAP = std::unordered_map<SESSION_ID, CPlayer*>;
using ACCOUNT_MAP = std::unordered_map<ACCNO, CPlayer*>;

#define __CHAT_LOGIN

#ifdef __CHAT_LOGIN
class CChatAsyncRedis;
#endif

class CChatServer :public CNetServer
{
public:

	friend class CChatAsyncRedis;

	struct stSectorPos
	{
		short shX;
		short shY;
	};


	CChatServer();
	~CChatServer();
	
	void StartChatServer(const WCHAR* pFileName = dfCHAT_SETTING_FILENAME);
	void StopChatServer(void);

private:

	static unsigned __stdcall JobExecuter(LPVOID parameter)
	{
		CChatServer* pThread = (CChatServer*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->JobThread(nullptr);
		return 0;
	}

	//////////////////////////////////////�ڵ鷯 : CNetClient ���������Լ� ������

	// connect/release
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
		StopChatServer();
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
	void OnError(int iErrorCode, WCHAR* wch) override{}
	void OnMonitoring(void) override;

	////////////////////////////////////////////////

	void SendPacket_Unicast(CPlayer* pUser, CPacket* pPacket);
	void SendPacket_Around(short shX, short shY, CPacket* pPacket);

    //�ʱ�ȭ(around sector ����Ʈ ����, pool ��)
	void InitSector(void);
	void InitChatVariable(void);

	//���� ���� �Լ�
	void AcceptUser(SESSION_ID iSessionID, CPacket* pPacket);
	void DisconnectUser(SESSION_ID iSessionID);
	void RemoveUserFromSector(CPlayer* pUser);

	//���� �˻� �Լ�
	CPlayer* FindUser(SESSION_ID iSessionId);
	CPlayer* AccountToPlayer(ACCNO iAccountNo);

	//���� ��Ŷ ó�� �Լ�
	void PacketProc(SESSION_ID iSessionID, CPacket* pPacket);
	void PacketProcLogin(SESSION_ID iSessionID, CPacket* pPacket);
	void PacketProcLoginCheck(SESSION_ID iSessionID, CPacket* pPacket);
	void PacketProcMoveSector(SESSION_ID iSessionID, CPacket* pPacket);
	void PacketProcChat(SESSION_ID iSessionID, CPacket* pPacket);
	void PacketProcHeartBeat(SESSION_ID iSessionID);

	//���� ��Ŷ �Լ�
	CPacket* mpLoginCheck(SESSION_ID iSessionID, ACCNO iAccountNo, char* pSessionKey);
	CPacket* mpLoginResponse(ACCNO iAccountNo, char chStatus);
	CPacket* mpMoveResponse(ACCNO iAccountNo, short shX, short shY);
	CPacket* mpChatResponse(CPlayer* pUser, short shLen, CPacket* pPacket);

	//������ �Լ�
	void		StartThread(void);
	void		StopThread(void);

	unsigned    JobThread(void* lpParam);

	void		MonitoringFunc(void);

	//////////////////////////////////////////////////////����
	WCHAR SettingFile[MAX_PATH];

#ifdef __CHAT_LOGIN
	CChatAsyncRedis* RedisClient;
	char	RedisIP[20];
	short	RedisPort;
#endif

	//���� ����
	int			 iMaxUser;
	PLAYER_MAP   PlayerMap;    //<SessionID, pUser>
	SRWLOCK      PlayerMapLock;
	ACCOUNT_MAP  AccountMap; //<ACCOUNTNO, pUser
	SRWLOCK      AccountMapLock;

	std::list<CPlayer*>		        Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
	std::vector<stSectorPos>		AroundSector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
	CTlsPool<CPlayer>*			ChatUserPool;

	DWORD       dwLastCheck;
	DWORD       dwTick;
	DWORD       dwTimeout;

	//�� ����
	CLockFreeQueue<stJob*>* JobQueue;
	CTlsPool<stJob>*        JobPool;
	HANDLE				    hJobEvent;

	//������ ����
	int    iJobThreadCount;
	HANDLE hThreadArr[dfCHATSERVER_MAX_THREADCOUNT];


	int iLoginCount;
	int iMoveCount;
	int iChatCount;
	int iHeartBeatCount;
	int iAroundSector;

	int iJobTPS;

	//����͸� �׸� ����

	CMonitoringClient MonitorClient;
	CHardwareStatus   HardwareStatus;
};