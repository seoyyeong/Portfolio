
#include "pch.h"
#include "CSession.h"
#include "Protocol.h"


using ACCNO = __int64;

class CPlayer
{
public:
	SESSION_ID  iSessionID;
	ACCNO       iAccountNo;

	short      shX;
	short      shY;

	unsigned short      shPort;

	WCHAR      ID[dfMAX_ID_LEN];
	WCHAR      NickName[dfMAX_NICKNAME_LEN];
	char       SessionKey[dfMAX_SESSIONKEY_LEN + 1];
	bool       bIsAlive;

	CPlayer()
	{
		iAccountNo = -1;
		iSessionID = -1;
		shX = -1;
		shY = -1;
		bIsAlive = FALSE;
		SessionKey[dfMAX_SESSIONKEY_LEN] = '\0';
	}
};