#pragma once

using ROOM_ID = __int64;

#define dfROOMTYPE_MAX 2

class CRoomMonitoring
{
public:
	int iSession;
	int iEnter;
	int iLeave;
	int iFPS;
	int iTPS;
};

//-----------------------------------룸 타입, 룸 관련 오류 리턴 정의
enum en_ROOM_TYPE
{
	en_ROOM_LOBBY = -1,
	en_ROOM_AUTH = 0,  //인증 스레드
	en_ROOM_ECHO = 1,  //에코 스레드
};

enum en_ROOM_ERROR
{
	en_ROOM_SUCCESS = 0,
	en_ROOM_RUNNING,
	en_ROOM_FULL,
	en_ROOM_UNEXISTING,
	en_ROOM_UNEXISTING_MOVE,
	en_ROOM_UNEXISTING_CUR,
	en_ROOM_CLOSED,
	en_ROOM_NO_SESSION
};