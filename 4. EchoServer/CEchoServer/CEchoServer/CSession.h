#pragma once

#include "pch.h"


#define dfSESSION_MAX_WSA 128

//#define __NETLOG_IO

using SESSION_ID = __int64;

class CSession;

struct stOverlapped
{
	enum OVERLAPPEDTYPE
	{
		SEND = 0x10, RECV
	};

	OVERLAPPED stOverlapped;
	CSession*  pSession;
	int		   iType;
};

class CSession
{
public:

	SESSION_ID iSessionID; //-1 : empty Session
	__int64	   iSessionidx;

	int          iIOCount;
	bool         bIsAlive;

	SOCKET			sock;
	unsigned short  shPort;
	u_long          lIP;

	CRingBuffer	    RecvQ;
	stOverlapped    stOverRecv;

	CLockFreeQueue<CPacket*>SendQ;
	bool			   bSendFlag;
	stOverlapped	   stOverSend;
	int                iSendPacket;
	CPacket*           SendPacketArr[dfSESSION_MAX_WSA];

	DWORD dwLastRecv;
	DWORD dwLastSend;
	DWORD dwTimeOut;
	DWORD dwReservedRelease;

#ifdef __NETLOG_IO
	struct stIOLog
	{
		SESSION_ID iSessionID;
		int iIOCount = 0;
		int iType = 0;
	};

	int iLogCount = 0;
	stIOLog Log[2000];
#endif

	CSession()
	{
		iSessionID = -1;

		iIOCount = 0x80000001;
		bSendFlag = FALSE;
		bIsAlive = FALSE;
		sock = INVALID_SOCKET;

		stOverRecv.pSession = this;
		stOverRecv.iType = stOverlapped::RECV;
		stOverSend.pSession = this;
		stOverSend.iType = stOverlapped::SEND;

		memset(&stOverRecv.stOverlapped, 0, sizeof(OVERLAPPED));
		memset(&stOverSend.stOverlapped, 0, sizeof(OVERLAPPED));
	}

};

enum en_SESSION_LOG
{
	NET_TIMEOUT_INC = 1,
	NET_TIMEOUT_DEC,

	NET_DISCONNECT_INC = 3,
	NET_DISCONNECT_DEC,

	NET_SENDPACKET_INC = 5,
	NET_SENDPACKET_DEC,

	NET_ACCEPT_INC = 7,
	NET_ACCEPT_DEC,

	NET_RECVPOST_INC = 9,
	NET_RECVPOST_DEC,

	NET_SENDPOST_INC = 11,
	NET_SENDPOST_DEC,

	NET_GQCS_DEC_RECV = 100,
	NET_GQCS_DEC_SEND,
	NET_GQCS_DEC_SENDPOST,

	NET_GQCS = 1000,

};