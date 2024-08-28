
#pragma once

#define dfMAX_ID_LEN	     20
#define dfMAX_NICKNAME_LEN   20
#define dfMAX_SESSIONKEY_LEN 64
#define dfMAX_MESSAGE_LEN    256

#pragma pack(1)

//Req : 클라이언트->서버 / Res : 서버->클라이언트

struct stGameReqLogin
{
	__int64 iAccountNo;
	char    SessionKey[dfMAX_SESSIONKEY_LEN];
	int     iVersion;
};

struct stGameResLogin
{
	char    chStatus;
	__int64 iAccountNo;
};

struct stGameReqEcho
{
	__int64 iAccountNo;
	__int64 iSendTick;
};

struct stGameResEcho
{
	__int64 iAccountNo;
	__int64 iSendTick;
};


#pragma pack()