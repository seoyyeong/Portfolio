#pragma once
#include "CRoomSession.h"


#define dfPLAYER_MAX_SENDPACKET 256

class CPlayer;

using ACCNO = __int64;
using ACCOUNT_MAP = std::unordered_map<ACCNO, SESSION_ID>;
using PLAYER_MAP = std::unordered_map<SESSION_ID, CPlayer*>;

class CPlayer
{
public:
	SESSION_ID   iSessionID;
	ACCNO        iAccountNo;
	ROOM_ID      iRoomType;
	short		 shPort;

	int          iSendPacket;
	CPacket*     SendPacketArr[dfPLAYER_MAX_SENDPACKET];

	CPlayer()
	{
		Initialize();
	}

	void Initialize()
	{
		iSessionID = -1;
		iAccountNo = -1;
		iSendPacket = 0;
		iRoomType = -1;
		shPort = 0;
	}
};