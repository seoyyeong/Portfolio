#pragma once

#include "pch.h"
#include "CommonProtocol.h"
#include "CEchoServer.h"
#include "CPlayer.h"
#include "CRoom.h"


class CEchoServer;

class CAuthRoomMonitoring :public CRoomMonitoring
{
public:
	int iAuthTPS;
	int iPool;
	int iMaxPool;
};

class CAuthRoom :public CRoom
{
public:

	
	CAuthRoom(CNetRoomServer* pServer, int iRoomIdx, int iMaxSession, DWORD dwFrame);
	~CAuthRoom(void) {}

	void Initialize(int count, va_list args) override { return;  }

private:

	void OnStart(void) override { iAuthTPS = 0; }
	void OnClose(void) override {}
	void OnTerminate(SESSION_ID iSessionID) override {}
	void OnError(int iLevel, const WCHAR* msg) override { return; }

	void OnRoomProc(void) override { return; }
	void OnEnter(SESSION_ID iSessionID) override;
	void OnLeave(SESSION_ID iSessionID) override;
	void OnMove(SESSION_ID iSessionID) override;
	void OnMoveFail(SESSION_ID iSessionID, ROOM_ID iMoveID, en_ROOM_ERROR err) override { return; }
	void OnDisconnect(SESSION_ID iSessionID) override;

	void OnRecv(SESSION_ID iSessionID, CPacket* pPacket) override;
	void OnGetMonitoring(CRoomMonitoring* param);
	void OnRefreshMonitor(void) override { iAuthTPS = 0; }

	inline CPlayer* FindPlayer(SESSION_ID iSessionID);

	void PacketProcAuth(CPlayer* pPlayer, CPacket* pPacket);
	bool CheckTokenValid(ACCNO iAccountNo, const char* pSessionKey);
	CPacket* mpLoginRes(char chStatus, ACCNO iAccountNo);

	CEchoServer* Server;
	PLAYER_MAP   PlayerMap;
	ACCOUNT_MAP  AccountMap;
	CObjectFreeList<CPlayer> PlayerPool;

	int iAuthTPS;
};