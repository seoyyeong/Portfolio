
#include "pch.h"
#include "CMonitoringServer.h"

CMonitoringServer::CMonitoringServer()
{
	InitializeSRWLock(&ServerLock);
	ServerPool = nullptr;
}

CMonitoringServer::~CMonitoringServer()
{
	if (bRunning)
	{
		StopMonitoringServer();
	}
	if (ServerPool != nullptr)
	{
		delete ServerPool;
	}
}

void CMonitoringServer::StartMonitoringServer(const WCHAR* FileName)
{

	Parse.LoadFile(FileName);

	InitDB();
	InitMonitoringServer();
	CSServer.StartMonitoringCSServer(FileName);
	hDBThread = (HANDLE)_beginthreadex(NULL, 0, DBExecuter, this, TRUE, NULL);
	StartServer();
}

void CMonitoringServer::StopMonitoringServer()
{
	StopServer();
}

void CMonitoringServer::InitDB(void)
{
	WCHAR IP[20];
	WCHAR Name[20];
	WCHAR Pw[20];
	WCHAR DbName[20];
	short shPort;

	Parse.GetValue(dfMONITORING_DBNAME, L"DB_IP", IP, 20);
	Parse.GetValue(dfMONITORING_DBNAME, L"DB_PORT", shPort);
	Parse.GetValue(dfMONITORING_DBNAME, L"DB_USERNAME", Name, 20);
	Parse.GetValue(dfMONITORING_DBNAME, L"DB_PASSWORD", Pw, 20);
	Parse.GetValue(dfMONITORING_DBNAME, L"DB_DBNAME", DbName, 20);
	Parse.GetValue(dfMONITORING_DBNAME, L"DB_UPDATETIME", dwDBUpdateTime);

	DB.InitConnection(IP, shPort, Name, Pw, DbName);

}

void CMonitoringServer::InitMonitoringServer(void)
{
	Parse.GetValue(dfMONITORING_SERVERNAME, L"SERVER_MAX", iMaxServer);
	ServerPool = new CObjectFreeList<stServer>(iMaxServer);

	iLoginCount = 0;
	iChatCount = 0;
	iGameCount = 0;
	iDBQueryCount = 0;


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

	Parse.GetValue(dfMONITORING_SERVERNAME, L"BIND_IP", ServerIP, 20);
	Parse.GetValue(dfMONITORING_SERVERNAME, L"BIND_PORT", ServerPort);
	Parse.GetValue(dfMONITORING_SERVERNAME, L"NAGLE", iNagle);
	Parse.GetValue(dfMONITORING_SERVERNAME, L"ZEROCOPYSEND", bZeroCopySend);

	Parse.GetValue(dfMONITORING_SERVERNAME, L"IOCP_WORKER_THREAD", iWorkerCnt);
	Parse.GetValue(dfMONITORING_SERVERNAME, L"IOCP_ACTIVE_THREAD", iActiveWorkerCnt);

	Parse.GetValue(dfMONITORING_SERVERNAME, L"SESSION_MAX", iMaxSession);

	Parse.GetValue(dfMONITORING_SERVERNAME, L"TIMEOUT_LOGIN", dwLoginTimeOut);
	Parse.GetValue(dfMONITORING_SERVERNAME, L"TIMEOUT_SENDRESERVE", dwReserveDisconnect);

	Parse.GetValue(dfMONITORING_SERVERNAME, L"CHECK_TIMEOUT", bCheckTimeOut);
	Parse.GetValue(dfMONITORING_SERVERNAME, L"RESERVE_DISCONNECT", bReserveDisconnect);

	InitServer(ServerIP, ServerPort, iNagle, bZeroCopySend, iWorkerCnt, iActiveWorkerCnt, iMaxSession, dwLoginTimeOut, dwReserveDisconnect, bCheckTimeOut, bReserveDisconnect);

}

void CMonitoringServer::OnClientAccept(SESSION_ID iSessionID, short shPort)
{
	stServer* pServer;

	pServer = ServerPool->Alloc();
	pServer->iSessionID = iSessionID;
	pServer->bIsAlive = FALSE;

	ServerMapLock();
	ServerMap.insert(std::make_pair(iSessionID, pServer));
	ServerMapUnLock();

}
void CMonitoringServer::OnClientLeave(SESSION_ID iSessionID)
{
	stServer* pServer;
	pServer = FindServer(iSessionID);

	ServerMapLock();
	ServerMap.erase(iSessionID);
	ServerMapUnLock();

	pServer->bIsAlive = FALSE;
	pServer->chType = -1;
	pServer->iSessionID = -1;

	ServerPool->Free(pServer);
}

void CMonitoringServer::OnRecv(SESSION_ID iSessionID, CPacket* pPacket)
{
	pPacket->AddRef();
	PacketProc(iSessionID, pPacket);
}



void CMonitoringServer::PacketProc(SESSION_ID iSessionID, CPacket* pPacket)
{
	short shType;

	if (pPacket->GetDataSize() < sizeof(shType))
	{
		pPacket->SubRef();
		return;
	}

	*pPacket >> shType;

	switch (shType)
	{
	case en_PACKET_SS_MONITOR_LOGIN:
		PacketProcLogin(iSessionID, pPacket);
		break;
	case en_PACKET_SS_MONITOR_DATA_UPDATE:
		PacketProcUpdate(iSessionID, pPacket);
		break;
	default:
		_LOG(L"MONITERING", CLog::LEVEL_ERROR, L"PacketProc : Code Error %hd(Session %lld)", shType, iSessionID);
		Disconnect(iSessionID);
		break;
	}

	pPacket->SubRef();
}

void CMonitoringServer::PacketProcLogin(SESSION_ID iSessionID, CPacket* pPacket)
{
	stServer* pServer;

	if (pPacket->GetDataSize() < sizeof(char))
	{
		_LOG(L"MONITERING", CLog::LEVEL_ERROR, L"PacketProcLogin : Packet Size %d", pPacket->GetDataSize());
		return;
	}

	pServer = FindServer(iSessionID);
	if (pServer == nullptr)
	{
		_LOG(L"MONITERING", CLog::LEVEL_ERROR, L"PacketProcLogin : Not Connected Server)");
		return;
	}

	*pPacket >> pServer->chType;
	
	switch (pServer->chType)
	{
	case TYPE_CHATSERVER:
	case TYPE_LOGINSERVER:
	case TYPE_GAMESERVER:
		break;
	default:
		_LOG(L"MONITERING", CLog::LEVEL_ERROR, L"PacketProcLogin : Type Error, Session %lld", iSessionID);
		Disconnect(iSessionID);
		break;
	}
}

void CMonitoringServer::PacketProcUpdate(SESSION_ID iSessionID, CPacket* pPacket)
{
	stServer* pServer;
	CPacket*  pSendPacket;
	char      chType;
	int		  iValue;
	int		  iTimeStamp;

	if (pPacket->GetDataSize() < sizeof(stMonitorSSData))
	{
		_LOG(L"MONITERING", CLog::LEVEL_ERROR, L"PacketProcUpdate : Packet Size %d", pPacket->GetDataSize());
		return;
	}

	pServer = FindServer(iSessionID);
	if (pServer == nullptr)
	{
		_LOG(L"MONITERING", CLog::LEVEL_ERROR, L"PacketProcUpdate : Not Connected Server)");
		return;
	}

	*pPacket >> chType >> iValue >> iTimeStamp;

	pSendPacket = mpMonitoringSCUpdate(pServer->chType, chType, iValue, iTimeStamp);
	if (pSendPacket == nullptr)
	{
		CRASH();
	}

	UpdateLog(pServer->chType, chType, iValue);

	switch (pServer->chType)
	{
	case TYPE_CHATSERVER:
		iChatCount++;
		break;

	case TYPE_LOGINSERVER:
		iLoginCount++;
		break;

	case TYPE_GAMESERVER:
		iGameCount++;
		break;
	default:
		_LOG(L"MONITERING", CLog::LEVEL_ERROR, L"PacketProcUpdate : ServerCode Error %c)",pServer->chType);
		break;
	}

	CSServer.SendPacketToAllClients(pSendPacket);
	pSendPacket->SubRef();

}

void CMonitoringServer::UpdateLog(char chServer, char chType, int iValue)
{

	DBLog[chType].Lock();
	
	DBLog[chType].bUse = TRUE;
	DBLog[chType].chServerType = chServer;
	DBLog[chType].chDataType = chType;
	DBLog[chType].iSum += iValue;
	
	if (DBLog[chType].iMaxValue < iValue)
	{
		DBLog[chType].iMaxValue = iValue;
	}
	if (DBLog[chType].iMinValue > iValue)
	{
		DBLog[chType].iMinValue = iValue;
	}

	DBLog[chType].iCount++;

	DBLog[chType].UnLock();

}

CPacket* CMonitoringServer::mpMonitoringSCUpdate(char chServer, char chType, int iValue, int iTimeStamp)
{
	CPacket* pSendPacket;

	pSendPacket = CPacket::Alloc();
	if (pSendPacket == nullptr)
	{
		return nullptr;
	}

	
	*pSendPacket << (short)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE << chServer << chType << iValue << iTimeStamp;

	return pSendPacket;
}


void CMonitoringServer::SendServerData(void)
{
	int iTimeStamp = (int)time(NULL);

	SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, HardwareStatus.ProcessorTotal(), iTimeStamp);
	SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, HardwareStatus.NonPagedPoolMB(), iTimeStamp);
	SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, HardwareStatus.InNetworkKB(), iTimeStamp);
	SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, HardwareStatus.OutNetworkKB(), iTimeStamp);
	SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, HardwareStatus.AvailableMemoryMB(), iTimeStamp);
}

void CMonitoringServer::SendMonitoringPacket(char chType, int iValue, int iTimeStamp)
{
	CPacket* pPacket;

	pPacket = mpMonitoringSCUpdate(-1, chType, iValue, iTimeStamp);

	CSServer.SendPacketToAllClients(pPacket);
	pPacket->SubRef();
	UpdateLog(-1, chType, iValue);

}

void CMonitoringServer::ServerMapLock(void)
{
	AcquireSRWLockExclusive(&ServerLock);

}

void CMonitoringServer::ServerMapUnLock(void)
{
	ReleaseSRWLockExclusive(&ServerLock);
}

CMonitoringServer::stServer* CMonitoringServer::FindServer(SESSION_ID iSessionID)
{
	std::unordered_map<SESSION_ID, stServer*>::iterator iter;
	stServer* pServer = nullptr;

	ServerMapLock();

	iter = ServerMap.find(iSessionID);
	if (iter != ServerMap.end())
	{
		pServer = iter->second;
	}

	ServerMapUnLock();

	return pServer;
}


void CMonitoringServer::OnMonitoring(void)
{
	SendServerData();

	wprintf(L"===================================================================================\n");
	CLog::PrintRunTimeLog();
	HardwareStatus.PrintHardwareStatus();
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"Connected Server %lld / Client %lld\n\n", ServerMap.size(), CSServer.ClientMap.size());
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"Recv SS TPS    : %-8d(%d bytes)\n", GetRecvTPS(), GetRecvBytesTPS());
	wprintf(L"- LoginServer  : %-8d \n", InterlockedExchange((long*)&iLoginCount, 0));
	wprintf(L"- ChatServer   : %-8d \n", InterlockedExchange((long*)&iChatCount, 0));
	wprintf(L"- GameServer   : %-8d\n", InterlockedExchange((long*)&iGameCount, 0));
	wprintf(L"Send CS TPS    : %-8d(%d bytes)\n", CSServer.GetSendTPS(), CSServer.GetSendBytesTPS());
	wprintf(L"Query   TPS    : %-8d\n", InterlockedExchange((long*)&iDBQueryCount, 0));
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"PacketPool     : %-8d / %-8d\n", CPacket::GetPacketPoolSize(), CPacket::GetPacketPoolTotalSize());
	wprintf(L"===================================================================================\n\n");

	CSServer.RefreshTPS();
}

unsigned CMonitoringServer::DBThread(void* lpParam)
{
	DWORD dwDelta;
	DWORD dwTick;
	DWORD dwSleepTime = 0;
	CFrame Frame(dwDBUpdateTime);

	dwTick = GetTime();

	while (bRunning)
	{
		Frame.FrameControl();

		DBFunc();

	}
	return 0;
}

void CMonitoringServer::DBFunc(void)
{
	bool   ret;
	float  fAvg;
	char   Query[300];

	time_t timer;
	tm     t;

	timer = time(NULL);
	localtime_s(&t, &timer);

	for (int i = 0; i < dfMONITORING_MAX_TYPECODE; i++)
	{
		DBLog[i].Lock();
		if (DBLog[i].bUse == TRUE)
		{

			if (DBLog[i].chDataType != dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN
				&& DBLog[i].chDataType != dfMONITOR_DATA_TYPE_GAME_SERVER_RUN
				&& DBLog[i].chDataType != dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN)
			{
				if (DBLog[i].iSum > 0 && DBLog[i].iCount > 0)
				{
					fAvg = (float)DBLog[i].iSum / DBLog[i].iCount;
				}
				else
				{
					fAvg = 0;
				}
			}
			else
			{
				fAvg = 0;
			}
			if (fAvg < DBLog[i].iMinValue && fAvg!=0)
			{
				int a = 0;
			}
			sprintf_s(Query, 300,
				"insert into `logdb`.monitorlog_%04d%02d(logtime, serverno, type, avg, min, max) values (now(), %d, %d, %.2f, %d, %d)",
				t.tm_year + 1900, t.tm_mon + 1,
				(int)DBLog[i].chServerType, (int)DBLog[i].chDataType, fAvg, DBLog[i].iMinValue, DBLog[i].iMaxValue);

			DBLog[i].Clear();
			DBLog[i].UnLock();

			ret = DB.Execute(Query);
			if (ret == FALSE)
			{
				CRASH();
			}
			InterlockedIncrement((long*)&iDBQueryCount);
		}
		else
		{
			DBLog[i].UnLock();
		}
	}

}