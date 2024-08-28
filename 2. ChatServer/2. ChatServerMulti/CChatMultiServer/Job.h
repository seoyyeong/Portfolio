#pragma once
#include "CPacket.h"

#define dfJOBTYPE_ACCEPT   1
#define dfJOBTYPE_LEAVE    2
#define dfJOBTYPE_RECV     3
#define dfJOBTYPE_SEND     4

#define dfJOBTYPE_CONNECT  0x88
#define dfJOBTYPE_SHUTDOWN 0x99


struct stJob
{
	short    shType;
	__int64  iSessionID;
	CPacket* pPacket;
};
