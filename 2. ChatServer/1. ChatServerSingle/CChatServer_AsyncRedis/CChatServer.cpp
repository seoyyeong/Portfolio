#include "pch.h"
#include "CChatServer.h"


CChatServer::CChatServer(void)
{
	ChatUserPool = nullptr;
	JobQueue = nullptr;
	JobPool = nullptr;
}

CChatServer::~CChatServer(void)
{
	if (bRunning)
	{
		StopChatServer();
	}

	if (ChatUserPool != nullptr)
	{
		delete ChatUserPool;
	}
	if (JobQueue != nullptr)
	{
		delete JobQueue;
	}
	if (JobPool != nullptr)
	{
		delete JobPool;
	}

}

void CChatServer::InitChatVariable(void)
{
	size_t  len;
	WCHAR   IP[20];

	Parse.GetValue(dfCHATSERVER_NAME, L"USER_MAX", iMaxUser);
	Parse.GetValue(dfCHATSERVER_NAME, L"TIMEOUT_CONTENTS", dwTimeout);

	ChatUserPool = new CTlsPool<CPlayer>(iMaxUser);
	JobQueue = new CLockFreeQueue<stJob*>(32 * MAX_BUCKETSIZE);
	JobPool = new CTlsPool<stJob>(32);
#ifdef __CHAT_LOGIN
	RedisClient = new CChatAsyncRedis(this);
#endif

	iLoginCount = 0;
	iMoveCount = 0;
	iChatCount = 0;
	iHeartBeatCount = 0;
	iAroundSector = 0;
	iJobTPS = 0;

	iJobThreadCount = 0;
	hJobEvent = (HANDLE)CreateEvent(NULL, FALSE, FALSE, NULL);


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


void CChatServer::StartThread(void)
{
	hThreadArr[iJobThreadCount++] = (HANDLE)_beginthreadex(NULL, 0, JobExecuter, this, true, NULL);
}

void CChatServer::StopThread(void)
{

	for (int i = 0; i < iJobThreadCount; i++)
	{
		stJob* pJob = JobPool->Alloc();
		pJob->shType = dfJOBTYPE_SHUTDOWN;
		JobQueue->Enqueue(pJob);
		SetEvent(hJobEvent);
	}

	WaitForMultipleObjects(iJobThreadCount, &hThreadArr[0], TRUE, INFINITE);

	for (int i = 0; i < iJobThreadCount; i++)
	{
		CloseHandle(hThreadArr[i]);
	}

	iJobThreadCount = 0;
}

void CChatServer::StartChatServer(const WCHAR* pFileName)
{
	Parse.LoadFile(pFileName);

	wcscpy_s(SettingFile, pFileName);

	InitChatVariable();
	InitSector();
#ifdef __CHAT_LOGIN
	RedisClient->Start();
#endif
	MonitorClient.Initialize(pFileName);
	MonitorClient.StartMonitoringClient();
	StartThread();

	StartServer();

}


void CChatServer::StopChatServer(void)
{
#ifdef __CHAT_LOGIN
	RedisClient->Stop();
#endif
	MonitorClient.StopMonitoringClient();
	StopThread();
	wprintf(L"ChatServer Stop : Session %lld / User %d\n", GetSessionCount(), PlayerMap.size());
}



void CChatServer::SendPacket_Unicast(CPlayer* pUser, CPacket* pPacket)
{
	if (pUser->bIsAlive == TRUE)
	{
		SendPacket(pUser->iSessionID, pPacket);
	}
}

void CChatServer::SendPacket_Around(short shX, short shY, CPacket* pPacket)
{
	std::vector<stSectorPos>::iterator PosIter;
	std::list<CPlayer*>::iterator   UserIter;

	std::vector<stSectorPos>& SendSector = AroundSector[shY][shX];

	for (PosIter = SendSector.begin(); PosIter != SendSector.end(); PosIter++)
	{
		for (UserIter = Sector[PosIter->shY][PosIter->shX].begin(); UserIter != Sector[PosIter->shY][PosIter->shX].end(); UserIter++)
		{
			SendPacket_Unicast(*UserIter, pPacket);
			iAroundSector++;
		}
	}

}

void CChatServer::OnClientAccept(SESSION_ID iSessionID, short shPort)
{
	stJob* pJob;
	CPacket* pPacket;

	pJob = JobPool->Alloc();
	pPacket = CPacket::Alloc();

	*pPacket << shPort;

	pJob->iSessionID = iSessionID;
	pJob->shType = dfJOBTYPE_ACCEPT;
	pJob->pPacket = pPacket;

	JobQueue->Enqueue(pJob);
	SetEvent(hJobEvent);
}

void CChatServer::OnClientLeave(SESSION_ID iSessionID)
{
	stJob* pJob;

	pJob = JobPool->Alloc();

	pJob->iSessionID = iSessionID;
	pJob->shType = dfJOBTYPE_LEAVE;

	JobQueue->Enqueue(pJob);
	SetEvent(hJobEvent);
}


void CChatServer::OnRecv(SESSION_ID iSessionID, CPacket* pPacket)
{
	stJob* pJob;

	pJob = JobPool->Alloc();
	pPacket->AddRef();

	pJob->iSessionID = iSessionID;
	pJob->shType = dfJOBTYPE_RECV;
	pJob->pPacket = pPacket;

	JobQueue->Enqueue(pJob);
	SetEvent(hJobEvent);
}


unsigned CChatServer::JobThread(void* lpParam)
{
	stJob* pJob;
	short  shType;
	bool   bIsRunning = TRUE;

	while (bIsRunning == TRUE)
	{
		WaitForSingleObject(hJobEvent, INFINITE);

		while (JobQueue->Dequeue(pJob))
		{
			shType = pJob->shType;
			JobPool->Free(pJob);

			switch (shType)
			{
			case dfJOBTYPE_ACCEPT:
				PRO_BEGIN("ACCEPT");
				AcceptUser(pJob->iSessionID, pJob->pPacket);
				PRO_END("ACCEPT");
				break;

			case dfJOBTYPE_LEAVE:
				PRO_BEGIN("DISCONNECT");
				DisconnectUser(pJob->iSessionID);
				PRO_END("DISCONNECT");
				break;

			case dfJOBTYPE_RECV:
				PacketProc(pJob->iSessionID, pJob->pPacket);
				break;

			case dfJOBTYPE_SHUTDOWN:
				bIsRunning = FALSE;
				break;

			default:
				_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"Chat JobThread : Invalid Type %d", pJob->shType);
				Disconnect(pJob->iSessionID);
			}

			iJobTPS++;

		}

	}
	return 0;
}

void CChatServer::AcceptUser(SESSION_ID iSessionID, CPacket* pPacket)
{
	CPlayer* pUser = ChatUserPool->Alloc();

	if (pUser->iAccountNo != -1 || pUser->iSessionID != -1 || pUser->bIsAlive == TRUE)
	{
		CRASH();
	}

	*pPacket >> pUser->shPort;
	pPacket->SubRef();

	pUser->iSessionID = iSessionID;

	PlayerMap.insert(std::make_pair(iSessionID, pUser));
}

void CChatServer::DisconnectUser(SESSION_ID iSessionID)
{
	CPlayer* pUser;
	PLAYER_MAP::iterator UserIter;

	UserIter = PlayerMap.find(iSessionID);
	if (UserIter == PlayerMap.end())
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"DisconnectChat : No sessionid %lld", iSessionID);
		return;
	}

	pUser = UserIter->second;

	RemoveUserFromSector(pUser);
	PlayerMap.erase(pUser->iSessionID);
	AccountMap.erase(pUser->iAccountNo);

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
		Sector[pUser->shY][pUser->shX].remove(pUser);
	}
}


//세션 검색 함수
CPlayer* CChatServer::FindUser(SESSION_ID iSessionID)
{
	PLAYER_MAP::iterator UserIter;

	UserIter = PlayerMap.find(iSessionID);
	if (UserIter == PlayerMap.end())
	{
		return nullptr;
	}

	return UserIter->second;

}

CPlayer* CChatServer::AccountToPlayer(ACCNO iAccountNo)
{
	ACCOUNT_MAP::iterator UserIter;

	UserIter = AccountMap.find(iAccountNo);
	if (UserIter == AccountMap.end())
	{
		return nullptr;
	}

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
		PRO_BEGIN("Login_req");
		PacketProcLogin(iSessionID, pPacket);
		PRO_END("Login_req");
		break;
#ifdef __CHAT_LOGIN
	case en_PACKET_CS_CHAT_REQ_LOGIN_CHECK:
		PRO_BEGIN("Login_res");
		PacketProcLoginCheck(iSessionID, pPacket);
		iJobTPS--;
		PRO_END("Login_res");
		break;
#endif
	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		PRO_BEGIN("Move");
		PacketProcMoveSector(iSessionID, pPacket);
		PRO_END("Move");
		break;

	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		PRO_BEGIN("Chat");
		PacketProcChat(iSessionID, pPacket);
		PRO_END("Chat");
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		PacketProcHeartBeat(iSessionID);
		break;
	}

	pPacket->SubRef();
}

void CChatServer::PacketProcLogin(SESSION_ID iSessionID, CPacket* pPacket)
{
	CPlayer* pUser;
	CPlayer* pOldUser;
	CPacket* pSendPacket;
	ACCNO    iAccountNo;

	if (pPacket->GetDataSize() < sizeof(stChatReqLogin))
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcLogin : Wrong PacketLen");
		Disconnect(iSessionID);
		return;
	}

	pUser = FindUser(iSessionID);
	if (pUser == nullptr)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcLogin : No User %lld", iSessionID);
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
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"Login Error : Duplicate Accno %lld(SessionId %lld)", iAccountNo, pOldUser->iSessionID);

		AccountMap.erase(iAccountNo);
		RemoveUserFromSector(pOldUser);

		pOldUser->bIsAlive = FALSE;
		pOldUser->shX = -1;
		pOldUser->shY = -1;

		Disconnect(pOldUser->iSessionID);
	}

	SetTimeOut(iSessionID, dwTimeout);

	pPacket->GetData((char*)pUser->ID, sizeof(WCHAR) * dfMAX_ID_LEN);
	pPacket->GetData((char*)pUser->NickName, sizeof(WCHAR) * dfMAX_NICKNAME_LEN);
	pPacket->GetData((char*)pUser->SessionKey, sizeof(char) * dfMAX_SESSIONKEY_LEN);

#ifdef __CHAT_LOGIN
	pSendPacket = mpLoginCheck(pUser->iSessionID, iAccountNo, pUser->SessionKey);
	RedisClient->Enqueue(pSendPacket);
#else
	pSendPacket = mpLoginResponse(pUser->iAccountNo, dfGAME_LOGIN_OK);
	if (pSendPacket == nullptr)
	{
		CRASH();
	}
	SendPacket_Unicast(pUser, pSendPacket);

	pSendPacket->SubRef();
#endif

	
	iLoginCount++;

}

void CChatServer::PacketProcLoginCheck(SESSION_ID iSessionID, CPacket* pPacket)
{
	CPlayer* pUser;
	CPacket* pSendPacket;
	char     chRes;

	if (pPacket->GetDataSize() < sizeof(ACCNO) + sizeof(chRes))
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcLoginCheck : Wrong PacketLen");
		Disconnect(iSessionID);
		return;
	}

	pUser = FindUser(iSessionID);
	if (pUser == nullptr)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcLogin : No User %lld", iSessionID);
		return;
	}

	*pPacket >> pUser->iAccountNo >> chRes;

	if (chRes == dfGAME_LOGIN_OK)
	{
		pUser->bIsAlive = TRUE;
		AccountMap.insert(std::make_pair(pUser->iAccountNo, pUser));
	}
	else
	{
		Disconnect(iSessionID);
		return;
	}

	pSendPacket = mpLoginResponse(pUser->iAccountNo, chRes);
	if (pSendPacket == nullptr)
	{
		CRASH();
	}
	SendPacket_Unicast(pUser, pSendPacket);

	pSendPacket->SubRef();

}

void CChatServer::PacketProcMoveSector(SESSION_ID iSessionID, CPacket* pPacket)
{
	CPlayer* pUser;
	CPacket* pSendPacket;
	ACCNO    iAccountNo;
	short       shX;
	short       shY;

	if (pPacket->GetDataSize() < sizeof(stChatReqSectorMove))
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcMove : Wrong PacketLen");
		Disconnect(iSessionID);
	}

	pUser = FindUser(iSessionID);
	if (pUser == nullptr)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcMove : No User %lld", iSessionID);
		return;
	}
	if (pUser->bIsAlive == FALSE)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcMove : Not Login %lld", iSessionID);
		Disconnect(iSessionID);
		return;
	}
	*pPacket >> iAccountNo;

	if (pUser->iAccountNo != iAccountNo)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcMove : Wrong Acc, User:%lld Recv:%lld", pUser->iAccountNo, iAccountNo);
		pUser->bIsAlive = FALSE;
		Disconnect(pUser->iSessionID);
		return;
	}

	SetTimeOut(iSessionID, dwTimeout);

	*pPacket >> shX >> shY;

	if (pUser->shX != shX && pUser->shY != shY)
	{
		RemoveUserFromSector(pUser);
		pUser->shX = shX;
		pUser->shY = shY;
		Sector[pUser->shY][pUser->shX].push_back(pUser);
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
	CPlayer* pUser;
	CPacket* pSendPacket;
	__int64           iAccountNo;
	short             shLen;

	*pPacket >> iAccountNo >> shLen;

	if (pPacket->GetDataSize() < shLen)
	{

		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcChat : Wrong PacketLen");
		Disconnect(iSessionID);
		return;
	}

	pUser = FindUser(iSessionID);
	if (pUser == nullptr)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcChat : No User %lld", iSessionID);
		return;
	}
	if (pUser->bIsAlive == FALSE)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcChat : Not Login %lld", iSessionID);
		Disconnect(iSessionID);
		return;
	}
	if (pUser->iAccountNo != iAccountNo)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcChat : Wrong Acc, User:%lld Recv:%lld", pUser->iAccountNo, iAccountNo);
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
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"PacketProcHeartBeat : No User %lld", iSessionID);
		return;
	}

	iHeartBeatCount++;

	SetTimeOut(iSessionID, dwTimeout);

}


//응답 패킷 함수

CPacket* CChatServer::mpLoginCheck(SESSION_ID iSessionID, ACCNO iAccountNo, char* pSessionKey)
{
	CPacket* pRet = CPacket::Alloc();

	if (pRet == nullptr)
	{
		return nullptr;
	}
	*pRet << iSessionID << iAccountNo;
	pRet->PutData(pSessionKey, dfMAX_SESSIONKEY_LEN);

	return pRet;
}

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
	iJobPoolSize = JobPool->GetAllocSize();
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
	wprintf(L"JobQueueSize   : %-8d\n", iJobPoolSize);
	wprintf(L"Job     TPS    : %-8d\n", iJob);
	wprintf(L"- Login        : %-8d \n", iLogin);
	wprintf(L"- Move         : %-8d \n", iMove);
	wprintf(L"- Chat         : %-8d\n", iChat);
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"PacketPool     : %-8d / %-8d\n", CPacket::GetPacketPoolTotalSize() - iPacketPoolSize, CPacket::GetPacketPoolTotalSize());
	wprintf(L"JobPool        : %-8d / %-8d\n", JobPool->GetTotalSize() - iJobPoolSize, JobPool->GetTotalSize());
	wprintf(L"ChatUserPool   : %-8d / %-8d\n", ChatUserPool->GetTotalSize() - ChatUserPool->GetAllocSize(), ChatUserPool->GetTotalSize());
	wprintf(L"===================================================================================\n\n");

	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, TRUE, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, iCPUProcess, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, iProcessMem, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_SESSION, iSession, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_PLAYER, iAccount, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, iJob, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, iPacketPoolSize, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, iJobPoolSize, iTimeStamp);
}


void CChatServer::InitSector(void)
{

	for (short x = 0; x < dfSECTOR_MAX_X; x++)
	{
		for (short y = 0; y < dfSECTOR_MAX_Y; y++)
		{
			stSectorPos Pos[9] =
			{
				{x - 1, y },
				{x - 1, y - 1},
				{x, y - 1},
				{x + 1, y - 1},
				{x + 1, y},
				{x + 1, y + 1},
				{x, y + 1},
				{x - 1, y + 1},
				{x, y}
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