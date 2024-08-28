#pragma once

#include "pch.h"
#include "CNetRoomServer.h"
#include "CRoomManager.h"
#include "CMonitoringClient.h"
#include "CPlayer.h"
#include "CAuthRoom.h"
#include "CEchoRoom.h"


#define dfECHO_SESSIONKEY_SIZE 64
#define dfECHOSERVER_NAME L"ECHO_SERVER"

class CRoomManager;
class CAuthRoom;
class CEchoRoom;


class CEchoServer : public CNetRoomServer
{
	friend CAuthRoom;
	friend CEchoRoom;

public:
	CEchoServer();
	~CEchoServer();
	void     Initialize(const WCHAR* FileName);

	ROOM_ID GetEchoRoomID(void) const
	{
		return iEchoRoomID;
	}
	ROOM_ID GetAuthRoomID(void) const
	{
		return iAuthRoomID;
	}

private:
	void OnServerStart(void) override;
	void OnServerClose(void) override;
	void OnServerControl(char ch) override {}
	void OnError(int iErrorCode, WCHAR* wch) override {}

	void OnClientAccept(SESSION_ID iSessionID, short shPort) override;
	void OnClientLeave(SESSION_ID iSessionID) override {}
	void OnSend(SESSION_ID iSessionID, int iSendSize) override {}
	void OnTimeOut(SESSION_ID iSessionID) override;

	void OnMonitoring(void);

	CMonitoringClient MonitorClient;

	CRoomManager*     pRoomManager;

	ROOM_ID iAuthRoomID;
	ROOM_ID iEchoRoomID;

	CHardwareStatus HardwareStatus;
	int iMaxPlayer;
	DWORD dwAuthSleep;
	DWORD dwEchoSleep;

	
};