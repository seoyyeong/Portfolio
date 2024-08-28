#pragma once

#define dfMAX_ID_LEN	     20
#define dfMAX_NICKNAME_LEN   20
#define dfMAX_SESSIONKEY_LEN 64
#define dfMAX_MESSAGE_LEN    256

#define dfSECTOR_MAX_X 50
#define dfSECTOR_MAX_Y 50


#pragma pack(1)

//Req : 클라이언트->서버 / Res : 서버->클라이언트

struct stChatReqLogin
{
	__int64 iAccountNo;
	WCHAR   ID[dfMAX_ID_LEN];
	WCHAR   NickName[dfMAX_NICKNAME_LEN];
	char    SessionKey[dfMAX_SESSIONKEY_LEN];
};

struct stChatResLogin
{
	char    chStatus;  //0 : 실패, 1 : 성공
	__int64 iAccountNo;
};

struct stChatReqSectorMove
{
	__int64 iAccountNo;
	short   shX;
	short   shY;
};

struct stChatResSectorMove
{
	short   shType;

	__int64 iAccountNo;
	short   shX;
	short   shY;
};

struct stChatReqMessage
{
	__int64 iAccountNo;
	short   shLen;
	WCHAR   Message[dfMAX_MESSAGE_LEN / 2];
};

struct stChatResMessage
{
	__int64 iAccountNo;
	WCHAR   ID[dfMAX_ID_LEN];
	WCHAR   NickName[dfMAX_NICKNAME_LEN];

	short   shLen;
	WCHAR   Message[dfMAX_MESSAGE_LEN / 2];
};



#pragma pack()