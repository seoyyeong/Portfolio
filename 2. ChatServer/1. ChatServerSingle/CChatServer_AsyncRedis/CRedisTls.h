#pragma once

#include "pch.h"

#include "Redis.h"


#define dfREDISTLS_MAX_POOLSIZE 64

class CRedisTls
{
public:
	CRedisTls(const std::string& host = "127.0.0.1", std::size_t port = 6379, int iPoolSize = dfREDISTLS_MAX_POOLSIZE)
	{
		dwTlsIndex = TLS_OUT_OF_INDEXES;
		iRedisCnt = 0;
		IP = host;
		shPort = port;


		dwTlsIndex = TlsAlloc();
		RedisPool = new CLockFreeList<cpp_redis::client>(iPoolSize);
	}
	~CRedisTls()
	{
		if (dwTlsIndex != TLS_OUT_OF_INDEXES)
		{
			TlsFree(dwTlsIndex);
		}
		for (int i = 0; i < iRedisCnt; i++)
		{
			RedisArr[i]->disconnect();
			RedisPool->Free(RedisArr[i]);
		}
		delete RedisPool;
	}
	__inline cpp_redis::client* GetClient(void)
	{
		cpp_redis::client* pClient;

		pClient = (cpp_redis::client*)TlsGetValue(dwTlsIndex);

		if (pClient == nullptr)
		{
			int idx = InterlockedIncrement((long*)&iRedisCnt) - 1;

			pClient = RedisPool->Alloc();
			RedisArr[idx] = pClient;

			pClient->connect(IP, (size_t)shPort);

			InterlockedIncrement((long*)&iConnect);
			TlsSetValue(dwTlsIndex, pClient);
		}
		return pClient;
	}

	std::future<cpp_redis::reply> Set(std::string& Key, std::string& Value)
	{
		cpp_redis::client* pClient;

		pClient = GetClient();

		return pClient->set(Key, Value);
	}
	std::future<cpp_redis::reply> SetEx(std::string& Key, int iSeconds, std::string& Value)
	{
		cpp_redis::client* pClient;

		pClient = GetClient();

		return pClient->setex(Key, iSeconds, Value);
	}
	std::future<cpp_redis::reply> Expire(std::string& Key, int iSecond)
	{
		cpp_redis::client* pClient;

		pClient = GetClient();

		return pClient->expire(Key, iSecond);
	}
	std::future<cpp_redis::reply> Get(std::string& Key)
	{
		cpp_redis::client* pClient;

		pClient = GetClient();

		return pClient->get(Key);
	}

	void Commit(void)
	{
		cpp_redis::client* pClient;

		pClient = GetClient();

		pClient->commit();
	}

	void Sync_Commit(void)
	{
		cpp_redis::client* pClient;

		pClient = GetClient();

		pClient->sync_commit();
	}


private:
	CLockFreeList<cpp_redis::client>* RedisPool;

	cpp_redis::client* RedisArr[dfREDISTLS_MAX_POOLSIZE];
	int                iRedisCnt;

	DWORD              dwTlsIndex;

	std::string        IP;
	short              shPort;

	int iConnect = 0;


};