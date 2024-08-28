#pragma once
#include "pch.h"
#include "CNetRoomServer.h"
#include "CRoomManager.h"

class CRoomManager;

class CRoom
{
public:

	CRoom(void* pServer, int iRoomIdx, int iMaxSession, DWORD dwFrame);
	virtual ~CRoom(void);

	static UINT WINAPI RoomExecuter(LPVOID param)
	{
		CRoom* pRoom = (CRoom*)param;
		CFrame Frame;
		Frame.SetFrame(pRoom->dwFrame);

		pRoom = (CRoom*)param;

		if (pRoom == NULL)
			return 0;

		while (pRoom->CheckAlive())
		{
			pRoom->RoomProc();
			pRoom->OnRoomProc();

			pRoom->iFPS++;
			Frame.FrameControl();
		}

		pRoom->Terminate();

		return 0;
	}


	HANDLE Start(void);
	void   Close(void);
	void   Terminate(void);
	virtual void Initialize(int count, va_list args) = 0;

	bool CheckAlive(void)
	{
		return InterlockedOr8((char*)&bIsRunning, 0);
	}
	bool CheckRoomFull(void)
	{
		return iMaxSession <= iSession;
	}
	ROOM_ID GetRoomID(void)
	{
		return iRoomID;
	}
	int     GetSessionCount(void)
	{
		return iSession;
	}
	void    GetMonitoring(void* param, bool bRefresh);

	en_ROOM_ERROR ReserveEnter(SESSION_ID iSessionID);

protected:

	en_ROOM_ERROR ReserveLeave(SESSION_ID iSessionID);
	en_ROOM_ERROR ReserveMove(ROOM_ID iMoveID, SESSION_ID iSessionID);
	en_ROOM_ERROR ReserveJob(SESSION_ID iSessionID, CPacket* pPacket);

	virtual void RoomProc(void);

	virtual void OnStart(void) = 0;
	virtual void OnClose(void) = 0;
	virtual void OnTerminate(SESSION_ID iSessionID) = 0;
	virtual void OnError(int iLevel, const WCHAR* msg) = 0;

	virtual void OnRoomProc(void) = 0;
	virtual void OnEnter(SESSION_ID iSessionID) = 0;
	virtual void OnLeave(SESSION_ID iSessionID) = 0;
	virtual void OnDisconnect(SESSION_ID iSessionID) = 0;
	virtual void OnMove(SESSION_ID iSessionID) = 0;
	virtual void OnMoveFail(SESSION_ID iSessionID, ROOM_ID iMoveRoom, en_ROOM_ERROR err) = 0;

	virtual void OnRecv(SESSION_ID iSessionID, CPacket* pPacket) = 0;

	virtual void OnGetMonitoring(CRoomMonitoring* param) = 0;
	virtual void OnRefreshMonitor(void) = 0;


	void RefreshMonitor(void);

	bool  bIsRunning;

	en_ROOM_TYPE    iRoomType;
	ROOM_ID         iRoomID;

private:
	
	CRoomSession* FindSession(SESSION_ID iSessionID);
	void          EnterProc(CRoomSession* pSession);
	void          LeaveProc(CRoomSession* pSession);
	void		  DisconnectProc(CRoomSession* pSession);
	void          MoveFailProc(CRoomSession* pSession, ROOM_ID iMoveRoom, en_ROOM_ERROR err);
	en_ROOM_ERROR MoveProc(CRoomSession* pSession, ROOM_ID iMoveRoomID);


	CPacket* mpLeavePacket(ROOM_ID iRoomID);
	CPacket* mpMovePacket(ROOM_ID iRoomID);
	CPacket* mpMoveFailPacket(ROOM_ID iMoveID, en_ROOM_ERROR iCode);

	CRoomManager*   pRoomManager;
	CNetRoomServer* pServer;
	HANDLE          hThread;

	DWORD           dwFrame;
	int             iMaxSession;

	int             iSession;
	int				iEnter;
	int				iLeave;
	int				iFPS;
	int				iTPS;

	ROOMSESSION_MAP SessionMap;
	CLockFreeQueue<CRoomSession*> EnterQueue;
	std::queue<CRoomSession*> LeaveQueue;


};