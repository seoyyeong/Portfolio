#include "pch.h"
#include "CEchoRoom.h"

CEchoRoom::CEchoRoom(void* pServer, int iRoomIdx, int iMaxSession, DWORD Frame) :CRoom(pServer, iRoomIdx, iMaxSession, Frame), PlayerPool(6000)
{
	Server = (CEchoServer*)pServer;
}

void CEchoRoom::OnRoomProc(void)
{
	PLAYER_MAP::iterator iter;
	CPlayer* pPlayer;

	for (iter = PlayerMap.begin(); iter != PlayerMap.end(); iter++)
	{
		pPlayer = iter->second;
		if (iter->second->iSendPacket > 0)
		{
			Server->SendPackets(pPlayer->iSessionID, pPlayer->SendPacketArr, pPlayer->iSendPacket);

			for (int i = 0; i < pPlayer->iSendPacket; i++)
			{
				pPlayer->SendPacketArr[i]->SubRef();
			}
			pPlayer->iSendPacket = 0;
		}
	}

}

void CEchoRoom::OnEnter(SESSION_ID iSessionID)
{
	PLAYER_MAP::iterator iter;
	CPlayer* pPlayer;
	CPacket* pPacket;

	pPlayer = PlayerPool.Alloc();
	pPlayer->iSessionID = iSessionID;
	PlayerMap.insert(std::make_pair(iSessionID, pPlayer));
}

void CEchoRoom::OnLeave(SESSION_ID iSessionID)
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

void CEchoRoom::OnDisconnect(SESSION_ID iSessionID)
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

void CEchoRoom::OnRecv(SESSION_ID iSessionID, CPacket* pPacket)
{
	CPlayer* pPlayer;

	pPlayer = FindPlayer(iSessionID);
	if (pPlayer != nullptr)
	{
		PacketProcEcho(pPlayer, pPacket);
	}
	else
	{
		_LOG(L"AUTH", CLog::LEVEL_ERROR, L"OnRecv Error : No Player");
		Server->Disconnect(iSessionID);
	}
	pPacket->SubRef();
}

void CEchoRoom::OnGetMonitoring(CRoomMonitoring* param)
{
	CEchoRoomMonitoring* st = (CEchoRoomMonitoring*)param;
	st->iEchoTPS = iEchoTPS;
	st->iPool = PlayerPool.GetPoolSize() - PlayerPool.GetAllocSize();
	st->iMaxPool = PlayerPool.GetPoolSize();
}

CPlayer* CEchoRoom::FindPlayer(SESSION_ID iSessionID)
{
	PLAYER_MAP::iterator iter;

	iter = PlayerMap.find(iSessionID);

	if (iter != PlayerMap.end())
	{
		return iter->second;
	}
	return nullptr;
}

void CEchoRoom::PacketProcEcho(CPlayer* pUser, CPacket* pPacket)
{
	CPacket* pSendPacket;

	short     shType;
	char    chStatus;
	ACCNO   iAccountNo;
	long long lSendTick;

	if (pPacket->GetDataSize() < sizeof(short))
	{
		_LOG(L"GAME", CLog::LEVEL_ERROR, L"PacketProc Error : Packet Size Error - No Type");
		Server->Disconnect(pUser->iSessionID);
		return;
	}
	*pPacket >> shType;

	switch (shType)
	{
		//AccountNo Setting
	case en_PACKET_CS_GAME_RES_LOGIN:
		*pPacket >> chStatus >> iAccountNo;
		pUser->iAccountNo = iAccountNo;
		pSendPacket = mpLoginRes(chStatus, iAccountNo);

		Server->SendPacket(pUser->iSessionID, pSendPacket);
		pSendPacket->SubRef();

		return;
		//Echo Logic
	case en_PACKET_CS_GAME_REQ_ECHO:
		if (pPacket->GetDataSize() < sizeof(stGameReqEcho))
		{
			_LOG(L"ECHO", CLog::LEVEL_ERROR, L"PacketProc Error : Packet Size Error - %d", pPacket->GetDataSize());
			Server->Disconnect(pUser->iSessionID);
			return;
		}
		Server->SetTimeOut(pUser->iSessionID, 3000);

		*pPacket >> iAccountNo >> lSendTick;
		if (pUser->iAccountNo == iAccountNo)
		{
			pSendPacket = mpEchoRes(iAccountNo, lSendTick);
			if (pSendPacket == nullptr)
			{
				_LOG(L"ECHO", CLog::LEVEL_ERROR, L"PacketProc Error : Packet Alloc Failed");
				CRASH();
			}
			pUser->SendPacketArr[pUser->iSendPacket++] = pSendPacket;
		}
		else
		{
			_LOG(L"GAME", CLog::LEVEL_ERROR, L"PacketProc Error : Wrong Accno(recv %lld, my %lld)", iAccountNo, pUser->iAccountNo);
			Server->Disconnect(pUser->iSessionID);
		}

		iEchoTPS++;

		break;

	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		Server->SetTimeOut(pUser->iSessionID, 3000);
		iHeartBeatTPS++;
		break;

	default:
		_LOG(L"GAME", CLog::LEVEL_ERROR, L"PacketProc Error : Wrong PacketType %d", shType);
		Server->Disconnect(pUser->iSessionID);
	}

}

CPacket* CEchoRoom::mpLoginRes(char chStatus, ACCNO iAccountNo)
{
	CPacket* pPacket = CPacket::Alloc();
	if (pPacket == nullptr)
	{
		return nullptr;
	}

	*pPacket << (short)en_PACKET_CS_GAME_RES_LOGIN << chStatus << iAccountNo;

	return pPacket;
}

CPacket* CEchoRoom::mpEchoRes(ACCNO iAccountNo, __int64 iSendTick)
{
	CPacket* pPacket = CPacket::Alloc();
	if (pPacket == nullptr)
	{
		return nullptr;
	}

	*pPacket << (short)en_PACKET_CS_GAME_RES_ECHO << iAccountNo << iSendTick;

	return pPacket;
}
