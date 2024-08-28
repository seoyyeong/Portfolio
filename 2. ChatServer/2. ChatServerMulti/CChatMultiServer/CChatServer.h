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


	//////////////////////////////////////핸들러 : CNetClient 순수가상함수 재정의

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

    //초기화(around sector 리스트 셋팅, pool 등)
	void InitSector(void);
	void InitChatVariable(void);

	//유저 관리 함수
	void AcceptUser(SESSION_ID iSessionID, short shPort);
	void DisconnectUser(SESSION_ID iSessionID);
	void RemoveUserFromSector(CPlayer* pUser);

	bool CheckToken(ACCNO iAccountNo, char* pSessionKey);

	//세션 검색 함수
	CPlayer* FindUser(SESSION_ID iSessionId);
	CPlayer* AccountToPlayer(ACCNO iAccountNo);

	//수신 패킷 처리 함수
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

	//응답 패킷 함수
	CPacket* mpLoginResponse(ACCNO iAccountNo, char chStatus);
	CPacket* mpMoveResponse(ACCNO iAccountNo, short shX, short shY);
	CPacket* mpChatResponse(CPlayer* pUser, short shLen, CPacket* pPacket);


	//////////////////////////////////////////////////////변수
	WCHAR SettingFile[MAX_PATH];

	CRedisTls    RedisClient;

	//유저 관리
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

	//모니터링 항목 관리

	CMonitoringClient MonitorClient;
	CHardwareStatus   HardwareStatus;

	int iLoginCount;
	int iMoveCount;
	int iChatCount;
	int iHeartBeatCount;
	int iAroundSector;

	int iJobTPS;

};