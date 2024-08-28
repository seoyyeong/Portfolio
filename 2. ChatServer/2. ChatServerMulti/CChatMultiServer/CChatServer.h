#pragma once

#include "CNetServer.h"
#include "CommonProtocol.h"
#include "CMonitoringClient.h"
#include "CPlayer.h"
#include "CRedisTls.h"

#define dfCHAT_SETTING_FILENAME L"ChatServer.cnf"
#define dfCHATSERVER_NAME       L"CHAT_SERVER"

//Login On
#define __CHAT_LOGIN

using PLAYER_MAP = std::unordered_map<SESSION_ID, CPlayer*>;
using ACCOUNT_MAP = std::unordered_map<ACCNO, CPlayer*>;


class CChatServer :public CNetServer
{
public:

	struct stSectorPos
	{
		short shX;
		short shY;
	};

	struct stSector
	{
		SRWLOCK        SectorLock;
		std::list<CPlayer*> PlayerList;
	};

	CChatServer();
	~CChatServer();
	
	void StartChatServer(const WCHAR* pFileName = dfCHAT_SETTING_FILENAME);
	void StopChatServer(void);

private:


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
	void OnMonitoring(void) override;
	void OnError(int iErrorCode, WCHAR* wch) override {}

	////////////////////////////////////////////////

	void SendPacket_Unicast(CPlayer* pUser, CPacket* pPacket);
	void SendPacket_Around(short shX, short shY, CPacket* pPacket);

    //�ʱ�ȭ(around sector ����Ʈ ����, pool ��)
	void InitSector(void);
	void InitChatVariable(void);

	//���� ���� �Լ�
	void AcceptUser(SESSION_ID iSessionID, short shPort);
	void DisconnectUser(SESSION_ID iSessionID);
	void RemoveUserFromSector(CPlayer* pUser);

	bool CheckToken(ACCNO iAccountNo, char* pSessionKey);

	//���� �˻� �Լ�
	CPlayer* FindUser(SESSION_ID iSessionId);
	CPlayer* AccountToPlayer(ACCNO iAccountNo);

	//���� ��Ŷ ó�� �Լ�
	void PacketProc(SESSION_ID iSessionID, CPacket* pPacket);
	void PacketProcLogin(SESSION_ID iSessionID, CPacket* pPacket);
	void PacketProcMoveSector(SESSION_ID iSessionID, CPacket* pPacket);
	void PacketProcChat(SESSION_ID iSessionID, CPacket* pPacket);
	void PacketProcHeartBeat(SESSION_ID iSessionID);

	void AcquireAroundSectorShared(short shX, short shY)
	{
		std::vector<stSectorPos>::iterator PosIter;

		for (PosIter = AroundSector[shY][shX].begin(); PosIter != AroundSector[shY][shX].end(); PosIter++)
		{
			AcquireSRWLockShared(&Sector[PosIter->shY][PosIter->shX].SectorLock);
		}
	}

	void ReleaseAroundSectorShared(short shX, short shY)
	{
		std::vector<stSectorPos>::iterator PosIter;

		for (PosIter = AroundSector[shY][shX].begin(); PosIter != AroundSector[shY][shX].end(); PosIter++)
		{
			ReleaseSRWLockShared(&Sector[PosIter->shY][PosIter->shX].SectorLock);
		}
	}

	//���� ��Ŷ �Լ�
	CPacket* mpLoginResponse(ACCNO iAccountNo, char chStatus);
	CPacket* mpMoveResponse(ACCNO iAccountNo, short shX, short shY);
	CPacket* mpChatResponse(CPlayer* pUser, short shLen, CPacket* pPacket);


	//////////////////////////////////////////////////////����
	WCHAR SettingFile[MAX_PATH];

	CRedisTls    RedisClient;

	//���� ����
	int			 iMaxUser;
	PLAYER_MAP   PlayerMap;    //<SessionID, pUser>
	SRWLOCK      PlayerMapLock;
	ACCOUNT_MAP  AccountMap; //<ACCOUNTNO, pUser
	SRWLOCK      AccountMapLock;

	stSector				        Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
	std::vector<stSectorPos>		AroundSector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
	CTlsPool<CPlayer>*			    ChatUserPool;

	DWORD       dwLastCheck;
	DWORD       dwTick;
	DWORD       dwTimeout;

	//����͸� �׸� ����

	CMonitoringClient MonitorClient;
	CHardwareStatus   HardwareStatus;

	int iLoginCount;
	int iMoveCount;
	int iChatCount;
	int iHeartBeatCount;
	int iAroundSector;

	int iJobTPS;

};