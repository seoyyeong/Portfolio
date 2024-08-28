#pragma once
#include "pch.h"

#define dfJOBTYPE_ACCEPT   0x01
#define dfJOBTYPE_LEAVE    0x02
#define dfJOBTYPE_RECV     0x03
#define dfJOBTYPE_SEND     0x04

#define dfJOBTYPE_ROOM_LEAVE          0x20
#define dfJOBTYPE_ROOM_DISCONNECT     0x21
#define dfJOBTYPE_ROOM_MOVE           0x22
#define dfJOBTYPE_ROOM_MOVEFAIL       0x23
#define dfJOBTYPE_ROOM_RECV           0x30
#define dfJOBTYPE_ROOM_JOB            0x31

#define dfJOBTYPE_CONNECT  0x88
#define dfJOBTYPE_SHUTDOWN 0x99


struct stJob
{
	short    shType;
	__int64  iSessionID;
	CPacket* pPacket;
};
