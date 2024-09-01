#include "pch.h"
#include "CChatServer.h"


CChatServer::CChatServer(void)
{
	ChatUserPool = nullptr;
}

CChatServer::~CChatServer(void)
{
	if (bRunning)
	{
		StopChatServer();
	}

}

void CChatServer::InitChatVariable(void)
{

	Parse.GetValue(dfCHATSERVER_NAME, L"USER_MAX", iMaxUser);
	Parse.GetValue(dfCHATSERVER_NAME, L"TIMEOUT_CONTENTS", dwTimeout);

	ChatUserPool = new CTlsPool<CPlayer>(iMaxUser);

	iLoginCount = 0;
	iMoveCount = 0;
	iChatCount = 0;
	iHeartBeatCount = 0;
	iAroundSector = 0;
	iJobTPS = 0;

	dwLastCheck = GetTime();

	/////////////////////////////////////Server Info
	WCHAR ServerIP[20];
	short ServerPort;
	int iNagle;
	bool bZeroCopySend;
	int iWorkerCnt;
	int iActiveWorkerCnt;
	int iMaxSession;
	DWORD dwLoginTimeOut;
	DWORD dwReserveDisconnect;
	bool bCheckTimeOut;
	bool bReserveDisconnect;
	char chHeaderCode;
	char chHeaderKey;


	Parse.GetValue(dfCHATSERVER_NAME, L"PACKET_CODE", (char*)&chHeaderCode, 1);
	Parse.GetValue(dfCHATSERVER_NAME, L"PACKET_KEY", (char*)&chHeaderKey, 1);
	CPacket::SetHeaderCode(chHeaderCode);
	CPacket::SetHeaderKey(chHeaderKey);

	Parse.GetValue(dfCHATSERVER_NAME, L"BIND_IP", ServerIP, 20);
	Parse.GetValue(dfCHATSERVER_NAME, L"BIND_PORT", ServerPort);
	Parse.GetValue(dfCHATSERVER_NAME, L"NAGLE", iNagle);
	Parse.GetValue(dfCHATSERVER_NAME, L"ZEROCOPYSEND", bZeroCopySend);

	Parse.GetValue(dfCHATSERVER_NAME, L"IOCP_WORKER_THREAD", iWorkerCnt);
	Parse.GetValue(dfCHATSERVER_NAME, L"IOCP_ACTIVE_THREAD", iActiveWorkerCnt);

	Parse.GetValue(dfCHATSERVER_NAME, L"SESSION_MAX", iMaxSession);

	Parse.GetValue(dfCHATSERVER_NAME, L"TIMEOUT_LOGIN", dwLoginTimeOut);
	Parse.GetValue(dfCHATSERVER_NAME, L"TIMEOUT_SENDRESERVE", dwReserveDisconnect);

	Parse.GetValue(dfCHATSERVER_NAME, L"CHECK_TIMEOUT", bCheckTimeOut);
	Parse.GetValue(dfCHATSERVER_NAME, L"RESERVE_DISCONNECT", bReserveDisconnect);

	InitServer(ServerIP, ServerPort, iNagle, bZeroCopySend, iWorkerCnt, iActiveWorkerCnt, iMaxSession, dwLoginTimeOut, dwReserveDisconnect, bCheckTimeOut, bReserveDisconnect);

}


void CChatServer::StartChatServer(const WCHAR* pFileName)
{
	Parse.LoadFile(pFileName);

	wcscpy_s(SettingFile, pFileName);
	MonitorClient.Initialize(pFileName);
	MonitorClient.StartMonitoringClient();
	InitSector();
	InitChatVariable();

	StartServer();
}


void CChatServer::StopChatServer(void)
{
	MonitorClient.StopMonitoringClient();
	wprintf(L"ChatServer Stop : Session %d / User %d\n",GetSessionCount(), PlayerMap.size());

	delete ChatUserPool;
}


void CChatServer::SendPacket_Unicast(CPlayer* pUser, CPacket* pPacket)
{
	SendPacket(pUser->iSessionID, pPacket);
}

void CChatServer::SendPacket_Around(short shX, short shY, CPacket* pPacket)
{
	std::vector<stSectorPos>::iterator PosIter;
	std::list<CPlayer*>::iterator   UserIter;

	std::vector<stSectorPos>& SendSector = AroundSector[shY][shX];

	AcquireAroundSectorShared(shX, shY);

	for (PosIter = SendSector.begin(); PosIter != SendSector.end(); PosIter++)
	{
		for (UserIter = Sector[PosIter->shY][PosIter->shX].PlayerList.begin(); UserIter != Sector[PosIter->shY][PosIter->shX].PlayerList.end(); UserIter++)
		{
			SendPacket_Unicast(*UserIter, pPacket);
			iAroundSector++;
		}
	}

	ReleaseAroundSectorShared(shX, shY);
}

void CChatServer::OnClientAccept(SESSION_ID iSessionID, short shPort)
{
	AcceptUser(iSessionID, shPort);
	InterlockedIncrement((long*)&iJobTPS);
}

void CChatServer::OnClientLeave(SESSION_ID iSessionID)
{
	DisconnectUser(iSessionID);
	InterlockedIncrement((long*)&iJobTPS);
}


void CChatServer::OnRecv(SESSION_ID iSessionID, CPacket* pPacket)
{
	PacketProc(iSessionID, pPacket);
	InterlockedIncrement((long*)&iJobTPS);
}


void CChatServer::AcceptUser(SESSION_ID iSessionID, short shPort)
{
	CPlayer* pUser = ChatUserPool->Alloc();

	if (pUser->iAccountNo != -1 || pUser->iSessionID != -1 || pUser->bIsAlive == TRUE)
	{
		CRASH();
	}

	pUser->iSessionID = iSessionID;
	pUser->shPort = shPort;

	AcquireSRWLockExclusive(&PlayerMapLock);
	PlayerMap.insert(std::make_pair(iSessionID, pUser));
	ReleaseSRWLockExclusive(&PlayerMapLock);
}

void CChatServer::DisconnectUser(SESSION_ID iSessionID)
{
	CPlayer* pUser;
	PLAYER_MAP::iterator UserIter;

	UserIter = PlayerMap.find(iSessionID);
	if (UserIter == PlayerMap.end())
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"DisconnectChat : No sessionid %lld", iSessionID);
		return;
	}

	pUser = UserIter->second;

	RemoveUserFromSector(pUser);

	AcquireSRWLockExclusive(&AccountMapLock);
	AccountMap.erase(pUser->iAccountNo);
	ReleaseSRWLockExclusive(&AccountMapLock);

	AcquireSRWLockExclusive(&PlayerMapLock);
	PlayerMap.erase(pUser->iSessionID);
	ReleaseSRWLockExclusive(&PlayerMapLock);

	pUser->iSessionID = -1;
	pUser->iAccountNo = -1;
	pUser->bIsAlive = FALSE;
	pUser->shX = -1;
	pUser->shY = -1;

	ChatUserPool->Free(pUser);

}

void CChatServer::RemoveUserFromSector(CPlayer* pUser)
{
	if (pUser->shX != -1 && pUser->shY != -1)
	{
		AcquireSRWLockExclusive(&Sector[pUser->shY][pUser->shX].SectorLock);
		Sector[pUser->shY][pUser->shX].PlayerList.remove(pUser);
		ReleaseSRWLockExclusive(&Sector[pUser->shY][pUser->shX].SectorLock);
	}
}


//세션 검색 함수
CPlayer* CChatServer::FindUser(SESSION_ID iSessionID)
{
	PLAYER_MAP::iterator UserIter;

	AcquireSRWLockShared(&PlayerMapLock);

	UserIter = PlayerMap.find(iSessionID);
	if (UserIter == PlayerMap.end())
	{
		ReleaseSRWLockShared(&PlayerMapLock);
		return nullptr;
	}

	ReleaseSRWLockShared(&PlayerMapLock);
	return UserIter->second;

}

CPlayer* CChatServer::AccountToPlayer(ACCNO iAccountNo)
{
	ACCOUNT_MAP::iterator UserIter;
	
	AcquireSRWLockShared(&AccountMapLock);

	UserIter = AccountMap.find(iAccountNo);
	if (UserIter == AccountMap.end())
	{
		ReleaseSRWLockShared(&AccountMapLock);
		return nullptr;
	}

	ReleaseSRWLockShared(&AccountMapLock);
	return UserIter->second;

}

//수신 패킷 처리 함수
void CChatServer::PacketProc(SESSION_ID iSessionID, CPacket* pPacket)
{
	short shType;

	*pPacket >> shType;

	switch (shType)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		PRO_BEGIN("LOGIN");
		PacketProcLogin(iSessionID, pPacket);
		PRO_END("LOGIN");
		break;

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		PRO_BEGIN("MOVE");
		PacketProcMoveSector(iSessionID, pPacket);
		PRO_END("MOVE");
		break;

	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		PRO_BEGIN("CHAT");
		PacketProcChat(iSessionID, pPacket);
		PRO_END("CHAT");
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		PacketProcHeartBeat(iSessionID);
		break;
	}

}

void CChatServer::PacketProcLogin(SESSION_ID iSessionID, CPacket* pPacket)
{
	CPlayer* pUser;
	CPlayer* pOldUser;
	CPacket* pSendPacket;
	ACCNO    iAccountNo;

	if (pPacket->GetDataSize() < sizeof(stChatReqLogin))
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"PacketProcLogin : Wrong PacketLen");
		Disconnect(iSessionID);
		return;
	}

	pUser = FindUser(iSessionID);
	if (pUser == nullptr)
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"PacketProcLogin : No User %lld", iSessionID);
		return;
	}

	if (pUser->bIsAlive == TRUE)
	{
		CRASH();
	}

	*pPacket >> iAccountNo;

	//중복로그인
	pOldUser = AccountToPlayer(iAccountNo);
	if (pOldUser != nullptr)
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"Login Error : Duplicate Accno %lld(SessionId %lld)", iAccountNo, pOldUser->iSessionID);

		RemoveUserFromSector(pOldUser);

		pOldUser->bIsAlive = FALSE;
		pOldUser->shX = -1;
		pOldUser->shY = -1;

		Disconnect(pOldUser->iSessionID);
	}

	SetTimeOut(iSessionID, dwTimeout);

	pUser->iAccountNo = iAccountNo;
	pPacket->GetData((char*)pUser->ID, sizeof(WCHAR) * dfMAX_ID_LEN);
	pPacket->GetData((char*)pUser->NickName, sizeof(WCHAR) * dfMAX_NICKNAME_LEN);
	pPacket->GetData(pUser->SessionKey, dfMAX_SESSIONKEY_LEN);

	if (CheckToken(pUser->iAccountNo, pUser->SessionKey) == FALSE)
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"Login Error : Invalid Token %lld(SessionId %lld)", iAccountNo, pUser->iSessionID);
		Disconnect(pUser->iSessionID);
		return;
	}

	pUser->bIsAlive = TRUE;

	AcquireSRWLockExclusive(&AccountMapLock);
	AccountMap.insert(std::make_pair(pUser->iAccountNo, pUser));
	ReleaseSRWLockExclusive(&AccountMapLock);

	pSendPacket = mpLoginResponse(pUser->iAccountNo, pUser->bIsAlive);
	if (pSendPacket == nullptr)
	{
		CRASH();
	}
	SendPacket_Unicast(pUser, pSendPacket);

	pSendPacket->SubRef();

	iLoginCount++;

}

bool CChatServer::CheckToken(ACCNO iAccountNo, char* pSessionKey)
{
#ifdef __CHAT_LOGIN
	std::string AccountNo = std::to_string(iAccountNo);
	std::string SessionKey = pSessionKey;
	std::string GetValue;
	std::future<cpp_redis::reply> get_reply;


	get_reply = RedisClient.get(AccountNo);
	RedisClient.sync_commit();

	GetValue = get_reply.get().as_string();
	if (GetValue.compare(SessionKey) == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
#endif
	return TRUE;
}

void CChatServer::PacketProcMoveSector(SESSION_ID iSessionID, CPacket* pPacket)
{
	CPlayer* pUser;
	CPacket* pSendPacket;
	ACCNO     iAccountNo;
	short       shX;
	short       shY;
	short       OldX;
	short       OldY;

	if (pPacket->GetDataSize() < sizeof(stChatReqSectorMove))
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"PacketProcMove : Wrong PacketLen");
		Disconnect(iSessionID);
		return;
	}

	pUser = FindUser(iSessionID);
	if (pUser == nullptr)
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"PacketProcMove : No User %lld", iSessionID);
		return;
	}
	if (pUser->bIsAlive == FALSE)
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"PacketProcMove : Not Login %lld", iSessionID);
		Disconnect(iSessionID);
		return;
	}
	*pPacket >> iAccountNo;

	if (pUser->iAccountNo != iAccountNo)
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"PacketProcMove : Wrong Acc, User:%lld Recv:%lld", pUser->iAccountNo, iAccountNo);
		pUser->bIsAlive = FALSE;
		Disconnect(pUser->iSessionID);
		return;
	}

	SetTimeOut(iSessionID, dwTimeout);

	*pPacket >> shX >> shY;

	OldX = pUser->shX;
	OldY = pUser->shY;

	if (OldX != shX && OldY != shY)
	{
		if (OldX == -1 && OldY == -1)
		{
			AcquireSRWLockExclusive(&Sector[shY][shX].SectorLock);

			pUser->shX = shX;
			pUser->shY = shY;
			Sector[shY][shX].PlayerList.push_back(pUser);

			ReleaseSRWLockExclusive(&Sector[shY][shX].SectorLock);
		}
		else
		{
			while (1)
			{
				if (TryAcquireSRWLockExclusive(&Sector[OldY][OldX].SectorLock))
				{
					if (TryAcquireSRWLockExclusive(&Sector[shY][shX].SectorLock))
					{
						Sector[OldY][OldX].PlayerList.remove(pUser);
						pUser->shX = shX;
						pUser->shY = shY;
						Sector[shY][shX].PlayerList.push_back(pUser);

						break;
					}

					ReleaseSRWLockExclusive(&Sector[OldY][OldX].SectorLock);
				}
				YieldProcessor();
			}

			ReleaseSRWLockExclusive(&Sector[shY][shX].SectorLock);
			ReleaseSRWLockExclusive(&Sector[OldY][OldX].SectorLock);
		}
	}

	pSendPacket = mpMoveResponse(pUser->iAccountNo, pUser->shX, pUser->shY);
	if (pSendPacket == nullptr)
	{
		CRASH();
	}
	SendPacket_Unicast(pUser, pSendPacket);
	pSendPacket->SubRef();

	iMoveCount++;
}

void CChatServer::PacketProcChat(SESSION_ID iSessionID, CPacket* pPacket)
{
	CPlayer*       pUser;
	CPacket*       pSendPacket;
	ACCNO          iAccountNo;
	short          shLen;

	*pPacket >> iAccountNo >> shLen;

	if (pPacket->GetDataSize() < shLen)
	{

		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"PacketProcChat : Wrong PacketLen");
		Disconnect(iSessionID);
		return;
	}

	pUser = FindUser(iSessionID);
	if (pUser == nullptr)
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"PacketProcChat : No User %lld", iSessionID);
		return;
	}
	if (pUser->bIsAlive == FALSE)
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"PacketProcChat : Not Login %lld", iSessionID);
		Disconnect(iSessionID);
		return;
	}
	if (pUser->iAccountNo != iAccountNo)
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"PacketProcChat : Wrong Acc, User:%lld Recv:%lld", pUser->iAccountNo, iAccountNo);
		pUser->bIsAlive = FALSE;
		Disconnect(pUser->iSessionID);
		return;
	}

	SetTimeOut(iSessionID, dwTimeout);

	pSendPacket = mpChatResponse(pUser, shLen, pPacket);

	if (pSendPacket == nullptr)
	{
		CRASH();
	}
	SendPacket_Around(pUser->shX, pUser->shY, pSendPacket);
	pSendPacket->SubRef();

	iChatCount++;
}

void CChatServer::PacketProcHeartBeat(SESSION_ID iSessionID)
{
	CPlayer* pUser;

	pUser = FindUser(iSessionID);
	if (pUser == nullptr)
	{
		_LOG(L"CHAT", CLog::LEVEL_ERROR, L"PacketProcHeartBeat : No User %lld", iSessionID);
		return;
	}

	iHeartBeatCount++;

	SetTimeOut(iSessionID, dwTimeout);

}


//응답 패킷 함수
CPacket* CChatServer::mpLoginResponse(ACCNO iAccountNo, char chStatus)
{
	CPacket* pRet = CPacket::Alloc();

	if (pRet == nullptr)
	{
		return nullptr;
	}
	*pRet << (short)en_PACKET_CS_CHAT_RES_LOGIN << chStatus << iAccountNo;
	
	return pRet;
}

CPacket* CChatServer::mpMoveResponse(ACCNO iAccountNo, short shX, short shY)
{
	CPacket* pRet = CPacket::Alloc();

	if (pRet == nullptr)
	{
		return nullptr;
	}
	*pRet << (short)en_PACKET_CS_CHAT_RES_SECTOR_MOVE << iAccountNo << shX << shY;

	return pRet;
}

CPacket* CChatServer::mpChatResponse(CPlayer* pUser, short shLen, CPacket* pPacket)
{
	CPacket* pRet = CPacket::Alloc();

	if (pRet == nullptr)
	{
		return nullptr;
	}
	*pRet << (short)en_PACKET_CS_CHAT_RES_MESSAGE << pUser->iAccountNo;

	pRet->PutData((char*)pUser->ID, sizeof(WCHAR) * dfMAX_ID_LEN);
	pRet->PutData((char*)pUser->NickName, sizeof(WCHAR) * dfMAX_NICKNAME_LEN);

	*pRet << shLen;

	pRet->PutData((char*)pPacket->GetReadPtr(), shLen);

	pPacket->MoveReadPos(shLen);

	return pRet;
}

void CChatServer::OnMonitoring(void)
{
	int    iTimeStamp;

	int    iCPUProcess;
	int    iProcessMem;
	int    iSession;
	int    iUser;
	int    iAccount;
	int    iJob;

	int    iAccept;
	int    iRelease;

	int iRecv;
	int iRecvB;
	int iSend;
	int iSendB;
	int iSendPacket;

	double dAroundAvg;
	int iLogin;
	int iMove;
	int iChat;

	int    iPacketPoolSize;
	int    iJobPoolSize;
	int    iUserPoolSize;

	iTimeStamp = (int)time(NULL);

	HardwareStatus.UpdateHardwareStatus();


	iCPUProcess = HardwareStatus.ProcessTotal();
	iProcessMem = HardwareStatus.UsedMemoryMB();
	iSession = GetSessionCount();
	iUser = PlayerMap.size();
	iAccount = AccountMap.size();
	iJob = InterlockedExchange((long*)&iJobTPS, 0);

	iAccept = GetAcceptTPS();
	iRelease = GetReleaseTPS();

	iRecv = GetRecvTPS();
	iRecvB = GetRecvBytesTPS();
	iSend = GetSendTPS();
	iSendB = GetSendBytesTPS();
	iSendPacket = GetSendMsgTPS();

	iLogin = InterlockedExchange((long*)&iLoginCount, 0);
	iMove = InterlockedExchange((long*)&iMoveCount, 0);
	iChat = InterlockedExchange((long*)&iChatCount, 0);

	if (iAroundSector == 0 || iChat == 0)
	{
		dAroundAvg = 0;
		iAroundSector = 0;
	}
	else
	{
		dAroundAvg = (double)iAroundSector / iChat;
		iAroundSector = 0;
	}

	iPacketPoolSize = CPacket::GetPacketPoolAllocSize();
	iUserPoolSize = ChatUserPool->GetAllocSize();


	wprintf(L"===================================================================================\n");
	CLog::PrintRunTimeLog();
	HardwareStatus.PrintHardwareStatus();
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"Session Count  : %d(ChatUser %d / Login %d)\n\n", iSession, iUser, iAccount);
	wprintf(L"Accept  TPS    : %-8d(Total %d)\n", iAccept, GetTotalAccept());
	wprintf(L"Release TPS    : %-8d(Total %d)\n", iRelease, GetTotalRelease());
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"Recv    TPS    : %-8d(%d bytes)\n", iRecv, iRecvB);
	wprintf(L"Send    TPS    : %-8d(%d bytes)\n", iSend, iSendB);
	wprintf(L"- Send Packet  : %-8d(Around AVG : %.2lf) \n", iSendPacket, dAroundAvg);
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"Job     TPS    : %-8d\n", iJob);
	wprintf(L"- Login        : %-8d \n", iLogin);
	wprintf(L"- Move         : %-8d \n", iMove);
	wprintf(L"- Chat         : %-8d\n", iChat);
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"PacketPool     : %-8d / %-8d\n", CPacket::GetPacketPoolTotalSize() - iPacketPoolSize, CPacket::GetPacketPoolTotalSize());
	wprintf(L"ChatUserPool   : %-8d / %-8d\n", ChatUserPool->GetTotalSize() - iUserPoolSize, ChatUserPool->GetTotalSize());
	wprintf(L"===================================================================================\n\n");

	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, TRUE, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, iCPUProcess, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, iProcessMem, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_SESSION, iSession, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_PLAYER, iAccount, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, iJob, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, iPacketPoolSize, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, 0, iTimeStamp);
}


void CChatServer::InitSector(void)
{

	for (short x = 0; x < dfSECTOR_MAX_X; x++)
	{
		for (short y = 0; y < dfSECTOR_MAX_Y; y++)
		{
			InitializeSRWLock(&Sector[y][x].SectorLock);

			stSectorPos Pos[9] =
			{
				{x, y},
				{x - 1, y - 1},
				{x, y - 1},
				{x + 1, y - 1},
				{x - 1, y },
				{x + 1, y},
				{x - 1, y + 1},
				{x, y + 1},
				{x + 1, y + 1}
			};

			for (int i = 0; i < 9; i++)
			{
				if (Pos[i].shX >= 0 && Pos[i].shX < dfSECTOR_MAX_X
					&& Pos[i].shY >= 0 && Pos[i].shY < dfSECTOR_MAX_Y)
				{
					AroundSector[y][x].push_back(Pos[i]);
				}
			
			}
		}
	}
}