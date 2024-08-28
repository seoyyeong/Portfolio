#include "pch.h"
#include "CAuthRoom.h"

CAuthRoom::CAuthRoom(CNetRoomServer* pServer, int iRoomIdx, int iMaxSession, DWORD dwFrame) :CRoom(pServer, iRoomIdx, iMaxSession, dwFrame), PlayerPool(6000)
{
	Server = (CEchoServer*)pServer;
}

void CAuthRoom::OnEnter(SESSION_ID iSessionID)
{
	CPlayer* pPlayer = PlayerPool.Alloc();
	if (pPlayer == nullptr)
	{
		CRASH();
	}
	pPlayer->iSessionID = iSessionID;
	pPlayer->iRoomType = iRoomType;
	pPlayer->iAccountNo = -1;
	PlayerMap.insert(std::make_pair(iSessionID, pPlayer));
}

void CAuthRoom::OnLeave(SESSION_ID iSessionID)
{
	CPlayer* pPlayer;
	PLAYER_MAP::iterator iter;
	
	iter = PlayerMap.find(iSessionID);

	if (iter == PlayerMap.end())
	{
		return;
	}
	pPlayer = iter->second;
	PlayerPool.Free(pPlayer);
	PlayerMap.erase(iter);
}

void CAuthRoom::OnMove(SESSION_ID iSessionID)
{
	CPacket* pPacket;
	PLAYER_MAP::iterator iter;

	iter = PlayerMap.find(iSessionID);

	if (iter == PlayerMap.end())
	{
		return;
	}

	pPacket = mpLoginRes(1, iter->second->iAccountNo);

	ReserveJob(iSessionID, pPacket);

}

void CAuthRoom::OnDisconnect(SESSION_ID iSessionID)
{
	CPlayer* pPlayer;
	PLAYER_MAP::iterator iter;

	iter = PlayerMap.find(iSessionID);

	if (iter == PlayerMap.end())
	{
		return;
	}
	pPlayer = iter->second;
	PlayerPool.Free(pPlayer);

	PlayerMap.erase(iter);
}

void CAuthRoom::OnRecv(SESSION_ID iSessionID, CPacket* pPacket)
{
	CPlayer* pPlayer;

	pPlayer = FindPlayer(iSessionID);
	if (pPlayer != nullptr)
	{
		PacketProcAuth(pPlayer, pPacket);
	}
	else
	{
		_LOG(L"AUTH", CLog::LEVEL_ERROR, L"OnRecv Error : No Player");
		Server->Disconnect(iSessionID);
	}
	pPacket->SubRef();
}

void CAuthRoom::OnGetMonitoring(CRoomMonitoring* param)
{
	CAuthRoomMonitoring* st = (CAuthRoomMonitoring*)param;
	st->iAuthTPS = iAuthTPS;
	st->iPool = PlayerPool.GetPoolSize() - PlayerPool.GetAllocSize();
	st->iMaxPool = PlayerPool.GetPoolSize();
}

CPlayer* CAuthRoom::FindPlayer(SESSION_ID iSessionID)
{
	PLAYER_MAP::iterator iter;

	iter = PlayerMap.find(iSessionID);

	if (iter != PlayerMap.end())
	{
		return iter->second;
	}
	return nullptr;
}

void CAuthRoom::PacketProcAuth(CPlayer* pPlayer, CPacket* pPacket)
{
	ACCOUNT_MAP::iterator iter;

	short       shType;
	ACCNO       iAccountNo;
	char        pSessionKey[dfECHO_SESSIONKEY_SIZE + 1];
	int         iVersion;

	if (pPacket->GetDataSize() < sizeof(shType))
	{
		_LOG(L"GAME", CLog::LEVEL_ERROR, L"PacketProcAuth Error : Packet Size Error - No Type");
		Server->Disconnect(pPlayer->iSessionID);
		return;
	}

	*pPacket >> shType;

	switch (shType)
	{
	case en_PACKET_CS_GAME_REQ_LOGIN:

		if (pPacket->GetDataSize() < sizeof(stGameReqLogin))
		{
			_LOG(L"GAME", CLog::LEVEL_ERROR, L"PacketProcAuth Error : Packet Size Error - %d", pPacket->GetDataSize());
			Server->Disconnect(pPlayer->iSessionID);
			return;
		}

		Server->SetTimeOut(pPlayer->iSessionID, 3000);

		*pPacket >> iAccountNo;

		iter = AccountMap.find(iAccountNo);
		if (iter != AccountMap.end())
		{
			_LOG(L"GAME", CLog::LEVEL_ERROR, L"PacketProcAuth Error : Existing Accno %lld", iAccountNo);
			Server->Disconnect(iter->second);
		}

		pPacket->GetData(pSessionKey, dfMAX_SESSIONKEY_LEN);
		*pPacket >> iVersion;

		if (!CheckTokenValid(iAccountNo, pSessionKey))
		{
			_LOG(L"GAME", CLog::LEVEL_ERROR, L"PacketProcAuth Error : SessionKey MisMatch");
			pPacket = mpLoginRes(0, pPlayer->iAccountNo);

			Server->SendPacket(pPlayer->iSessionID, pPacket);

			Server->Disconnect(pPlayer->iSessionID);
		}
		else
		{
			pPlayer->iAccountNo = iAccountNo;

			AccountMap.insert(std::make_pair(iAccountNo, pPlayer->iSessionID));

			ReserveMove(Server->GetEchoRoomID(), pPlayer->iSessionID);

			break;
		}


	default:
		_LOG(L"GAME", CLog::LEVEL_ERROR, L"PacketProcAuth Error : Wrong PacketType %hd", shType);
		Server->Disconnect(pPlayer->iSessionID);
	}

	iAuthTPS++;
}

bool CAuthRoom::CheckTokenValid(ACCNO iAccountNo, const char* pSessionKey)
{
	return TRUE;
}

CPacket* CAuthRoom::mpLoginRes(char chStatus, ACCNO iAccountNo)
{
	CPacket* pPacket = CPacket::Alloc();
	if (pPacket == nullptr)
	{
		return nullptr;
	}

	*pPacket << (short)en_PACKET_CS_GAME_RES_LOGIN << chStatus << iAccountNo;

	return pPacket;
}