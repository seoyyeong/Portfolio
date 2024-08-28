#include "pch.h"
#include "CRoom.h"


CRoom::CRoom(void* pServer, int iRoomIdx, int iMaxSession, DWORD dwFrame)
{
	this->pServer = (CNetRoomServer*)pServer;
	this->iMaxSession = iMaxSession;
	this->dwFrame = dwFrame;
	this->iRoomID = (iRoomIdx << dfROOMINDEX_SHIFT) | (__int64)this;
	this->pRoomManager = CRoomManager::GetInstance();
}

CRoom::~CRoom(void)
{
	if (CheckAlive())
	{
		Close();
	}
}

HANDLE CRoom::Start(void)
{
	if (InterlockedExchange8((char*)&bIsRunning, TRUE) == FALSE)
	{
		RefreshMonitor();
		hThread = (HANDLE)_beginthreadex(NULL, 0, RoomExecuter, this, TRUE, NULL);
		OnStart();
	}
	return hThread;
}

void CRoom::Close(void)
{
	InterlockedExchange8((char*)&bIsRunning, FALSE);
}

void CRoom::Terminate(void)
{

	ROOMSESSION_MAP::iterator iter;
	CRoomSession* pSession;

	while (EnterQueue.Dequeue(pSession))
	{
		OnTerminate(pSession->iSessionID);
	}

	while (!LeaveQueue.empty())
	{
		pSession = LeaveQueue.front();
		LeaveQueue.pop();
		LeaveProc(pSession);
	}

	for (iter = SessionMap.begin(); iter != SessionMap.end(); iter++)
	{
		pSession = iter->second;

		if (pSession->iCurRoom != iRoomID)
		{
			continue;
		}
		OnTerminate(pSession->iSessionID);
		
	}
	
	OnClose();
}

void CRoom::GetMonitoring(void* param, bool bRefresh)
{
	CRoomMonitoring* p = (CRoomMonitoring*)param;

	p->iEnter = iEnter;
	p->iFPS = iFPS;
	p->iTPS = iTPS;
	p->iSession = iSession;
	p->iLeave = iLeave;

	OnGetMonitoring(p);

	if (bRefresh)
	{
		RefreshMonitor();
	}

}
void CRoom::RefreshMonitor(void)
{
	iEnter = 0;
	iLeave = 0;
	iFPS = 0;
	iTPS = 0;

	OnRefreshMonitor();
}

en_ROOM_ERROR CRoom::ReserveEnter(SESSION_ID iSessionID)
{
	CRoomSession* pSession;

	pSession = pServer->AcquireSession(iSessionID);
	if (pSession == nullptr)
	{
		return en_ROOM_NO_SESSION;
	}
	
	EnterQueue.Enqueue(pSession);
	return en_ROOM_SUCCESS;
}


en_ROOM_ERROR CRoom::ReserveLeave(SESSION_ID iSessionID)
{
	CRoomSession* pSession;

	pSession = pServer->AcquireSession(iSessionID);
	if (pSession == nullptr)
	{
		return en_ROOM_NO_SESSION;
	}

	LeaveQueue.push(pSession);
	pServer->ReturnSession(pSession);

	return en_ROOM_SUCCESS;
}

en_ROOM_ERROR CRoom::ReserveMove(ROOM_ID iMoveID, SESSION_ID iSessionID)
{
	CRoomSession* pSession;
	stJob* pJob;

	pSession = pServer->AcquireSession(iSessionID);
	if (pSession == nullptr)
	{
		return en_ROOM_NO_SESSION;
	}

	pJob = pServer->JobPool.Alloc();
	pJob->shType = dfJOBTYPE_ROOM_MOVE;
	pJob->iSessionID = iSessionID;
	pJob->pPacket = mpMovePacket(iMoveID);

	pSession->JobQueue.Enqueue(pJob);
	pServer->ReturnSession(pSession);

	return en_ROOM_SUCCESS;
}


en_ROOM_ERROR CRoom::ReserveJob(SESSION_ID iSessionID, CPacket* pPacket)
{
	CRoomSession* pSession;
	stJob* pJob;

	pSession = FindSession(iSessionID);
	if (pSession == nullptr)
	{
		return en_ROOM_NO_SESSION;
	}

	pJob = pServer->JobPool.Alloc();
	pJob->shType = dfJOBTYPE_ROOM_RECV;
	pJob->iSessionID = iSessionID;
	pJob->pPacket = pPacket;

	pSession->JobQueue.Enqueue(pJob);

	return en_ROOM_SUCCESS;
}

void CRoom::RoomProc(void)
{
	ROOMSESSION_MAP::iterator iter;
	CRoomSession* pSession;
	stJob*   pJob;
	ROOM_ID  iID;
	en_ROOM_ERROR err;

	while (EnterQueue.Dequeue(pSession))
	{
		EnterProc(pSession);
	}

	while (!LeaveQueue.empty())
	{
		pSession = LeaveQueue.front();
		LeaveQueue.pop();
		LeaveProc(pSession);
	}

	for (iter = SessionMap.begin(); iter != SessionMap.end(); iter++)
	{
		pSession = iter->second;

		if (pSession->iCurRoom != iRoomID)
		{
			continue;
		}
		while (pSession->JobQueue.Dequeue(pJob))
		{
			if (pJob->iSessionID != iter->second->iSessionID)
			{
				CRASH();
			}

			switch (pJob->shType)
			{
			case dfJOBTYPE_ROOM_LEAVE:
				LeaveQueue.push(pSession);
				break;

			case dfJOBTYPE_ROOM_DISCONNECT:
				pSession->iCurRoom = -1;
				LeaveQueue.push(pSession);
				break;

			case dfJOBTYPE_ROOM_MOVE:
				*pJob->pPacket >> iID;
				MoveProc(pSession, iID);
				pJob->pPacket->SubRef();
				break;

			case dfJOBTYPE_ROOM_MOVEFAIL:
				*pJob->pPacket >> iID >> (int&)err;
				MoveFailProc(pSession, iID, err);
				pJob->pPacket->SubRef();
				break;
			case dfJOBTYPE_ROOM_RECV:
				OnRecv(pSession->iSessionID, pJob->pPacket);
				break;
			default:
				OnError(CLog::LEVEL_ERROR, L"RoomProc Error : JobType Error");
				pServer->Disconnect(pSession->iSessionID);
				break;
			}

			pServer->JobPool.Free(pJob);

			if (pSession->iCurRoom != iRoomID)
			{
				break;
			}
			iTPS++;
		}
	}
}

CRoomSession* CRoom::FindSession(SESSION_ID iSessionID)
{
	ROOMSESSION_MAP::iterator iter;

	iter = SessionMap.find(iSessionID);
	if (iter != SessionMap.end())
	{
		return iter->second;
	}
	return nullptr;
}

void CRoom::EnterProc(CRoomSession* pSession)
{
	stJob* pJob;

	if (CheckRoomFull())
	{
		pJob = pServer->JobPool.Alloc();
		pJob->iSessionID = pSession->iSessionID;
		pJob->shType = dfJOBTYPE_ROOM_MOVEFAIL;
		pJob->pPacket = mpMoveFailPacket(iRoomID, en_ROOM_FULL);

		pSession->JobQueue.Enqueue(pJob);

		pSession->iCurRoom = pSession->iPrevRoom;
		for (int i = 0; i < 10; i++)
		{
			if (pRoomManager->InsertSession(pSession->iSessionID, pSession->iPrevRoom) == en_ROOM_SUCCESS)
			{
				return;
			}
		}
		pServer->Disconnect(pSession->iSessionID);
	}
	else
	{
		pSession->iCurRoom = iRoomID;
		SessionMap.insert(std::make_pair(pSession->iSessionID, pSession));
		iEnter++;
		iSession++;
		OnEnter(pSession->iSessionID);
	}

}

void CRoom::LeaveProc(CRoomSession* pSession)
{
	pSession->iPrevRoom = iRoomID;
	SessionMap.erase(pSession->iSessionID);
	iLeave++;
	iSession--;
	if (pSession->iCurRoom == -1)
	{
		OnDisconnect(pSession->iSessionID);
	}
	else
	{
		OnLeave(pSession->iSessionID);
	}
	pServer->ReturnSession(pSession);
}

void CRoom::DisconnectProc(CRoomSession* pSession)
{
	pSession->iPrevRoom = -1;
	SessionMap.erase(pSession->iSessionID);
	iLeave++;
	iSession--;
	OnDisconnect(pSession->iSessionID);
	pServer->ReturnSession(pSession);
}

void CRoom::MoveFailProc(CRoomSession* pSession, ROOM_ID iMoveRoom, en_ROOM_ERROR err)
{
	OnMoveFail(pSession->iSessionID, iMoveRoom, err);
}

en_ROOM_ERROR CRoom::MoveProc(CRoomSession* pSession, ROOM_ID iMoveRoomID)
{
	en_ROOM_ERROR ret;
	stJob* pJob;

	pSession->iPrevRoom = iRoomID;
	pSession->iCurRoom = iMoveRoomID;

	ret = pRoomManager->InsertSession(iMoveRoomID, pSession->iSessionID);
	if (ret == en_ROOM_SUCCESS)
	{
		LeaveQueue.push(pSession);
		OnMove(pSession->iSessionID);
	}
	else
	{
		pJob = pServer->JobPool.Alloc();
		pJob->iSessionID = pSession->iSessionID;
		pJob->shType = dfJOBTYPE_ROOM_MOVEFAIL;
		pJob->pPacket = mpMoveFailPacket(iMoveRoomID, ret);

		pSession->JobQueue.Enqueue(pJob);
	}
	return ret;
}

CPacket* CRoom::mpLeavePacket(ROOM_ID iRoomID)
{
	CPacket* pPacket = CPacket::Alloc();
	if (pPacket == nullptr)
	{
		return nullptr;
	}

	*pPacket << iRoomID;
	return pPacket;
}

CPacket* CRoom::mpMovePacket(ROOM_ID iMoveID)
{
	CPacket* pPacket = CPacket::Alloc();
	if (pPacket == nullptr)
	{
		return nullptr;
	}

	*pPacket << iMoveID;
	return pPacket;
}

CPacket* CRoom::mpMoveFailPacket(ROOM_ID iMoveID, en_ROOM_ERROR iCode)
{
	CPacket* pPacket = CPacket::Alloc();
	if (pPacket == nullptr)
	{
		return nullptr;
	}

	*pPacket << iMoveID << iCode;
	return pPacket;

}