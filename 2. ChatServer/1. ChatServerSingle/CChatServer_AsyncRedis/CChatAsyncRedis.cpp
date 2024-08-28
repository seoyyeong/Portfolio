#include "pch.h"
#include "CChatAsyncRedis.h"

void CChatAsyncRedis::RedisProc(CPacket* pPacket)
{
	SESSION_ID  iSessionID;
	ACCNO       iAccountNo;
	char        Key[dfMAX_SESSIONKEY_LEN + 1];
	char        chRes;
	stJob* pJob;

	*pPacket >> iSessionID >> iAccountNo;
	pPacket->GetData(Key, dfMAX_SESSIONKEY_LEN);

	chRes = CheckToken(iAccountNo, Key);

	*pPacket << (short)en_PACKET_CS_CHAT_REQ_LOGIN_CHECK << iAccountNo << chRes;

	pJob = pServer->JobPool->Alloc();

	pJob->iSessionID = iSessionID;
	pJob->shType = dfJOBTYPE_RECV;
	pJob->pPacket = pPacket;

	pServer->JobQueue->Enqueue(pJob);
	SetEvent(pServer->hJobEvent);
}


char CChatAsyncRedis::CheckToken(ACCNO iAccountNo, char* pSessionKey)
{
	std::string AccountNo = std::to_string(iAccountNo);
	std::string SessionKey = pSessionKey;
	std::string GetValue;
	std::future<cpp_redis::reply> get_reply;
	cpp_redis::reply ret;

	get_reply = Client.get(AccountNo);
	Client.sync_commit();

	ret = get_reply.get();
	if (ret.is_null())
	{
		return FALSE;
	}

	GetValue = ret.as_string();
	if (GetValue.compare(SessionKey) == 0)
	{
		return dfGAME_LOGIN_OK;
	}
	else
	{
		return dfGAME_LOGIN_FAIL;
	}

}