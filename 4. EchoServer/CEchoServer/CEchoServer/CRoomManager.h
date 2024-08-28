#pragma once
#include "pch.h"
#include "RoomProtocol.h"
#include "CRoom.h"

#define dfROOMINDEX_SHIFT 17

class CRoom;

class CRoomManager
{
public:
	using ROOM_INFO = std::pair<HANDLE, CRoom*>;
	using ROOM_MAP = std::unordered_map<ROOM_ID, ROOM_INFO>;

	static CRoomManager* GetInstance(void);

	ROOM_ID			 CreateRoom(en_ROOM_TYPE iType, void* pServer, int iMaxSession, DWORD dwFrame);
	en_ROOM_ERROR    DeleteRoom(ROOM_ID iRoomID);
	void		     Initialize(ROOM_ID iRoomID, int count, ...);
	en_ROOM_ERROR    Start(ROOM_ID iID);
	en_ROOM_ERROR    Close(ROOM_ID iID);

	en_ROOM_ERROR    InsertSession(ROOM_ID iID, SESSION_ID iSessionID);
	en_ROOM_ERROR    GetMonitoring(ROOM_ID iID, void* param, bool bRefresh);

	CRoomManager(const CRoomManager&) = delete;
	CRoomManager& operator=(const CRoomManager&) = delete;

private:

	CRoomManager(void);
	~CRoomManager(void);
	void Release(void);


	static CRoomManager* pInst;
	static std::mutex    Mutex;

	ROOM_MAP RoomMap;
	SRWLOCK  RoomMapLock;
	int      iRoomIdx;
	int      iRoomCount;


};