#pragma once

#include "pch.h"
#include "CommonProtocol.h"
#include "CEchoServer.h"
#include "CRoom.h"
#include "CPlayer.h"


class CEchoServer;

class CEchoRoomMonitoring :public CRoomMonitoring
{
public:
	int iEchoTPS;
	int iHeartBeatTPS;
	int iPool;
	int iMaxPool;
};

class CEchoRoom :public CRoom
{

public:

	CEchoRoom() = delete;
	CEchoRoom(void* pServer, int iRoomIdx, int iMaxSession, DWORD Frame);
	~CEchoRoom(void) {}

	void Initialize(int count, va_list args) override {}

private:

	void OnStart(void) override { iEchoTPS = 0; iHeartBeatTPS = 0; }
	void OnClose(void) override {}
	void OnTerminate(SESSION_ID iSessionID) override {}
	void OnError(int iLevel, const WCHAR* msg) override {}

	void OnRoomProc(void) override;
	void OnEnter(SESSION_ID iSessionID) override;
	void OnLeave(SESSION_ID iSessionID) override;
	void OnDisconnect(SESSION_ID iSessionID) override;
	void OnMove(SESSION_ID iSessionID) override {}
	void OnMoveFail(SESSION_ID iSessionID, ROOM_ID iMoveRoom, en_ROOM_ERROR err) override {}

	void OnRecv(SESSION_ID iSessionID, CPacket* pPacket) override;
	void OnGetMonitoring(CRoomMonitoring* param) override;
	void OnRefreshMonitor(void) override { iEchoTPS = 0; }

	CPlayer* FindPlayer(SESSION_ID iSessionID);

	void PacketProcEcho(CPlayer* pPlayer, CPacket* pPacket);

	CPacket* mpLoginRes(char chStatus, ACCNO iAccountNo);
	CPacket* mpEchoRes(ACCNO iAccountNo, __int64 iSendTick);

	CEchoServer* Server;
	PLAYER_MAP   PlayerMap;
	CObjectFreeList<CPlayer> PlayerPool;

	int iEchoTPS;
	int iHeartBeatTPS;

};