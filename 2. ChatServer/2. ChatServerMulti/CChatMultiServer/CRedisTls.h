#pragma once

#include "pch.h"

#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")


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

	__inline std::future<cpp_redis::reply> set(std::string& Key, std::string& Value)
	{
		cpp_redis::client* pClient;

		pClient = GetClient();

		return pClient->set(Key, Value);
	}
	__inline std::future<cpp_redis::reply> setex(std::string& Key, int iSeconds, std::string& Value)
	{
		cpp_redis::client* pClient;

		pClient = GetClient();

		return pClient->setex(Key, iSeconds, Value);
	}
	__inline std::future<cpp_redis::reply> expire(std::string& Key, int iSecond)
	{
		cpp_redis::client* pClient;

		pClient = GetClient();

		return pClient->expire(Key, iSecond);
	}
	__inline std::future<cpp_redis::reply> get(std::string& Key)
	{
		cpp_redis::client* pClient;

		pClient = GetClient();

		return pClient->get(Key);
	}

	__inline void commit(void)
	{
		cpp_redis::client* pClient;

		pClient = GetClient();

		pClient->commit();
	}

	__inline void sync_commit(void)
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