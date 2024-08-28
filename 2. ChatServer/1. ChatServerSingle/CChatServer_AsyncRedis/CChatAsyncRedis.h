#pragma once
#include "CAsyncRedis.h"
#include "CChatServer.h"

class CChatServer;

class CChatAsyncRedis :public CAsyncRedis
{
public:

	CChatAsyncRedis(CChatServer* pParam)
	{
		pServer = pParam;
	}

private:

	void RedisProc(CPacket* pPacket);

	char CheckToken(ACCNO iAccountNo, char* pSessionKey);

    CChatServer* pServer;

};