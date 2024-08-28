#include "pch.h"
#include "CRoomManager.h"
#include "RoomConstructor.h"

CRoomManager* CRoomManager::pInst;
std::mutex CRoomManager::Mutex;

CRoomManager::CRoomManager(void)
{
	InitializeSRWLock(&RoomMapLock);
	iRoomIdx = 0;
}

CRoomManager::~CRoomManager(void)
{
	Release();
}

void CRoomManager::Release(void)
{
	ROOM_MAP::iterator iter;
	HANDLE* hThreads;
	int idx = 0;
	int size;
	
	AcquireSRWLockExclusive(&RoomMapLock);
	size = RoomMap.size();
	hThreads = new HANDLE[size];

	for (iter = RoomMap.begin(); iter != RoomMap.end(); iter++)
	{
		hThreads[idx] = iter->second.first;
		iter->second.second->Close();
		idx++;
	}

	WaitForMultipleObjects(size, hThreads, TRUE, INFINITE);
	RoomMap.clear();

	for (int i = 0; i < size; i++)
	{
		CloseHandle(hThreads[idx]);
	}
	ReleaseSRWLockExclusive(&RoomMapLock);
	delete[] hThreads;
	delete pInst;
}

CRoomManager* CRoomManager::GetInstance(void)
{
	if (pInst == nullptr)
	{
		Mutex.lock();
		if (pInst == nullptr)
		{
			pInst = new CRoomManager;
		}
		Mutex.unlock();
	}

	return pInst;
}

en_ROOM_ERROR CRoomManager::InsertSession(ROOM_ID iID, SESSION_ID iSessionID)
{
	CRoom*        pRoom;
	en_ROOM_ERROR ret;
	ROOM_MAP::iterator iter;

	AcquireSRWLockShared(&RoomMapLock);

	iter = RoomMap.find(iID);
	if (iter != RoomMap.end())
	{
		pRoom = iter->second.second;
		if (pRoom->CheckAlive() == TRUE)
		{
			ret = pRoom->ReserveEnter(iSessionID);
		}
		else
		{
			ret = en_ROOM_CLOSED;
		}

	}
	else
	{
		ret = en_ROOM_UNEXISTING;
	}
	ReleaseSRWLockShared(&RoomMapLock);

	return ret;
}

en_ROOM_ERROR CRoomManager::GetMonitoring(ROOM_ID iID, void* param, bool bRefresh)
{
	CRoom* pRoom;
	en_ROOM_ERROR ret;
	ROOM_MAP::iterator iter;

	AcquireSRWLockShared(&RoomMapLock);

	iter = RoomMap.find(iID);
	if (iter != RoomMap.end())
	{
		pRoom = iter->second.second;
		if (pRoom->CheckAlive() == TRUE)
		{
			pRoom->GetMonitoring(param, bRefresh);
		}
		else
		{
			ret = en_ROOM_CLOSED;
		}

	}
	else
	{
		ret = en_ROOM_UNEXISTING;
	}
	ReleaseSRWLockShared(&RoomMapLock);

	return ret;
}

en_ROOM_ERROR CRoomManager::Start(ROOM_ID iID)
{
	CRoom* pRoom;
	en_ROOM_ERROR ret;
	ROOM_MAP::iterator iter;


	AcquireSRWLockShared(&RoomMapLock);

	iter = RoomMap.find(iID);
	if (iter != RoomMap.end())
	{
		pRoom = iter->second.second;
		if (pRoom->CheckAlive() == FALSE)
		{
			iter->second.first = pRoom->Start();
			ret = en_ROOM_SUCCESS;
		}
		else
		{
			ret = en_ROOM_RUNNING;
		}

	}
	else
	{
		ret = en_ROOM_UNEXISTING;
	}
	ReleaseSRWLockShared(&RoomMapLock);

	return ret;
}

en_ROOM_ERROR CRoomManager::Close(ROOM_ID iID)
{
	CRoom* pRoom;
	HANDLE hThread;
	en_ROOM_ERROR   ret;
	ROOM_MAP::iterator iter;

	AcquireSRWLockShared(&RoomMapLock);

	iter = RoomMap.find(iID);
	if (iter != RoomMap.end())
	{
		hThread = iter->second.first;
		pRoom = iter->second.second;
		if (pRoom->CheckAlive() == TRUE)
		{
			pRoom->Close();
			WaitForSingleObject(hThread, INFINITE);
			ret = en_ROOM_SUCCESS;
			CloseHandle(hThread);
		}
		else
		{
			ret = en_ROOM_CLOSED;
		}
		RoomMap.erase(iter);

	}
	else
	{
		ret = en_ROOM_UNEXISTING;
	}

	ReleaseSRWLockShared(&RoomMapLock);

	InterlockedDecrement((long*)&iRoomCount);
	return ret;
}

ROOM_ID CRoomManager::CreateRoom(en_ROOM_TYPE iType, void* pServer, int iMaxSession, DWORD dwFrame)
{
	CRoom*    pRoom;
	int       iIdx;
	ROOM_ID   iRoomID;

	iIdx = InterlockedIncrement((long*)&iRoomIdx);

	pRoom = RoomConstructors[iType](pServer, iIdx, iMaxSession, dwFrame);
	iRoomID = pRoom->GetRoomID();

	AcquireSRWLockExclusive(&RoomMapLock);
	RoomMap.insert(std::make_pair(iRoomID, std::make_pair((HANDLE)NULL, pRoom)));
	ReleaseSRWLockExclusive(&RoomMapLock);

	InterlockedIncrement((long*)&iRoomCount);

	return iRoomID;

}

en_ROOM_ERROR CRoomManager::DeleteRoom(ROOM_ID iRoomID)
{
	CRoom* pRoom;
	HANDLE hThread;
	en_ROOM_ERROR   ret;
	ROOM_MAP::iterator iter;

	AcquireSRWLockShared(&RoomMapLock);

	iter = RoomMap.find(iRoomID);
	if (iter != RoomMap.end())
	{
		hThread = iter->second.first;
		pRoom = iter->second.second;
		if (pRoom->CheckAlive() == TRUE)
		{
			pRoom->Close();
			WaitForSingleObject(hThread, INFINITE);
			ret = en_ROOM_SUCCESS;
		}

		CloseHandle(hThread);
		delete pRoom;
		RoomMap.erase(iter);

	}
	else
	{
		ret = en_ROOM_UNEXISTING;
	}

	ReleaseSRWLockShared(&RoomMapLock);

	InterlockedDecrement((long*)&iRoomCount);
	return ret;

}

void CRoomManager::Initialize(ROOM_ID iRoomID, int count, ...)
{
	va_list args;
	CRoom*  pRoom;
	ROOM_MAP::iterator iter;

	va_start(args, count);

	AcquireSRWLockShared(&RoomMapLock);
	iter = RoomMap.find(iRoomID);
	if (iter != RoomMap.end())
	{
		pRoom = iter->second.second;
		pRoom->Initialize(count - 1, args);
	}
	else
	{
		_LOG(L"RoomManager", CLog::LEVEL_ERROR, L"Initialize : No Room %lld\n", iRoomID);
	}
	ReleaseSRWLockShared(&RoomMapLock);

	va_end(args);
}
