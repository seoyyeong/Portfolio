#pragma once


#include "Job.h"
#include "CSession.h"
#include "RoomProtocol.h"


class CRoomSession;

using ROOMSESSION_MAP = std::unordered_map<SESSION_ID, CRoomSession*>;

class CRoomSession:public CSession
{
public:


	ROOM_ID      iCurRoom;
	ROOM_ID      iPrevRoom;

	alignas(64)CLockFreeQueue<stJob*>     JobQueue;

	CRoomSession()
	{
		iCurRoom = en_ROOM_LOBBY;
		iPrevRoom = en_ROOM_LOBBY;
	}
};