#pragma once

#include <pch.h>
#include "Redis.h"

class CAsyncRedis
{
public:
	enum enREDIS_JOBTYPE
	{
		enREDIS_SET,
		enREDIS_GET
	};

	CAsyncRedis(const std::string& host = "127.0.0.1", std::size_t port = 6379)
	{
		
		Client.connect(host, port);

		bRunning = FALSE;

		hThread = NULL;
		hExitEvent = (HANDLE)CreateEvent(NULL, TRUE, FALSE, NULL);
		hJobEvent = (HANDLE)CreateEvent(NULL, TRUE, FALSE, NULL);
		
		hEvent[0] = hExitEvent;
		hEvent[1] = hJobEvent;

	}

	~CAsyncRedis()
	{
		if (bRunning == TRUE)
		{
			Stop();
		}
	}

	void Start(void)
	{
		hThread = (HANDLE)_beginthreadex(NULL, 0, Executer, this, true, NULL);
		bRunning = TRUE;
	}

	void Stop(void)
	{
		SetEvent(hExitEvent);

		WaitForSingleObject(hThread, INFINITE);

		Client.disconnect();
		CloseHandle(hThread);
		CloseHandle(hExitEvent);
		CloseHandle(hJobEvent);
	}

	int GetJobQueuesize(void)
	{
		return JobQueue.Size();
	}

	bool Enqueue(CPacket* pPacket)
	{
		int ret;

		ret = JobQueue.Enqueue(pPacket);

		SetEvent(hJobEvent);
		return ret;
	}

	bool Dequeue(CPacket* pPacket)
	{
		return JobQueue.Dequeue(pPacket);
	}

	std::future<cpp_redis::reply> Set(std::string& Key, std::string& Value)
	{
		return Client.set(Key, Value);
	}
	std::future<cpp_redis::reply> SetEx(std::string& Key, int iSeconds, std::string& Value)
	{
		return Client.setex(Key, iSeconds, Value);
	}
	std::future<cpp_redis::reply> Expire(std::string& Key, int iSecond)
	{
		return Client.expire(Key, iSecond);
	}
	std::future<cpp_redis::reply> Get(std::string& Key)
	{
		return Client.get(Key);
	}

	void Commit(void)
	{
		Client.commit();
	}

	void Sync_Commit(void)
	{
		Client.sync_commit();
	}

private:
	static unsigned __stdcall Executer(LPVOID parameter)
	{
		CAsyncRedis* pThread = (CAsyncRedis*)parameter;
		if (pThread == NULL)
			return 0;

		pThread->Loop();
		return 0;
	}

	void Loop(void)
	{
		CPacket* pPacket;
		DWORD    ret;

		while (1)
		{
			ret = WaitForMultipleObjects(2, &hEvent[0], FALSE, INFINITE);

			if (ret == WAIT_OBJECT_0)
			{
				break;
			}

			while (JobQueue.Dequeue(pPacket))
			{
				RedisProc(pPacket);
			}
		}

	}

	virtual void RedisProc(CPacket* pPacket) = 0;


	CLockFreeQueue<CPacket*> JobQueue;

	bool    bRunning;
	HANDLE	hEvent[2];
	HANDLE	hJobEvent;
	HANDLE	hExitEvent;
	HANDLE  hThread;

protected:
	cpp_redis::client Client;
};