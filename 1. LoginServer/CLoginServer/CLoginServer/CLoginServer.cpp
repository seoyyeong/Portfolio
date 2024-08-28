#include "pch.h"
#include "CLoginServer.h"

CLoginServer::CLoginServer()
{

}

CLoginServer::~CLoginServer()
{
	if (bRunning == TRUE)
	{
		StopLoginServer();
	}
}

void CLoginServer::InitLoginVariable(void)
{
	Parse.GetValue(dfLOGIN_SERVERNAME, L"GAMESERVER_IP", GameServerIP, 20);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"GAMESERVER_PORT", GameServerPort);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"CHATSERVER_IP", ChatServerIP, 20);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"CHATSERVER_PORT", ChatServerPort);
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


	Parse.GetValue(dfLOGIN_SERVERNAME, L"PACKET_CODE", (char*)&chHeaderCode, 1);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"PACKET_KEY", (char*)&chHeaderKey, 1);
	CPacket::SetHeaderCode(chHeaderCode);
	CPacket::SetHeaderKey(chHeaderKey);

	Parse.GetValue(dfLOGIN_SERVERNAME, L"BIND_IP", ServerIP, 20);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"BIND_PORT", ServerPort);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"NAGLE", iNagle);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"ZEROCOPYSEND", bZeroCopySend);

	Parse.GetValue(dfLOGIN_SERVERNAME, L"IOCP_WORKER_THREAD", iWorkerCnt);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"IOCP_ACTIVE_THREAD", iActiveWorkerCnt);

	Parse.GetValue(dfLOGIN_SERVERNAME, L"SESSION_MAX", iMaxSession);

	Parse.GetValue(dfLOGIN_SERVERNAME, L"TIMEOUT_LOGIN", dwLoginTimeOut);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"TIMEOUT_SENDRESERVE", dwReserveDisconnect);

	Parse.GetValue(dfLOGIN_SERVERNAME, L"CHECK_TIMEOUT", bCheckTimeOut);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"RESERVE_DISCONNECT", bReserveDisconnect);

	InitServer(ServerIP, ServerPort, iNagle, bZeroCopySend, iWorkerCnt, iActiveWorkerCnt, iMaxSession, dwLoginTimeOut, dwReserveDisconnect, bCheckTimeOut, bReserveDisconnect);

}

void CLoginServer::InitDB(void)
{

	WCHAR   IP[20];
	WCHAR   UserName[20];
	WCHAR   Pw[20];
	WCHAR   Name[20];
	short   Port;
	size_t  len;

	Parse.GetValue(dfLOGIN_SERVERNAME, L"DB_IP", IP, 20);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"DB_PORT", Port);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"DB_USERNAME", UserName, 20);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"DB_PASSWORD", Pw, 20);
	Parse.GetValue(dfLOGIN_SERVERNAME, L"DB_DBNAME", Name, 20);

	wcstombs_s(&len, DBName, Name, 20);
	DBConnector.InitConnection(IP, Port, UserName, Pw, Name);
}

void CLoginServer::StartLoginServer(const WCHAR* FileName)
{
	Parse.LoadFile(FileName);

	MonitorClient.Initialize(FileName);
	MonitorClient.StartMonitoringClient();
	InitLoginVariable();
	InitDB();
#ifndef _LOGIN_CONNECTIONPOOL
	RedisClient.connect();
	InitializeSRWLock(&RedisLock);
#endif
	StartServer();
}

void CLoginServer::StopLoginServer(void)
{
	MonitorClient.StopMonitoringClient();
	DBConnector.CloseConnection();
	wprintf(L"LoginServer Stop : Session count %d", GetSessionCount());
}

void CLoginServer::OnRecv(SESSION_ID iSessionID, CPacket *pPacket)
{
	pPacket->AddRef();
	PacketProc(iSessionID, pPacket);
}

void CLoginServer::PacketProc(SESSION_ID iSessionID, CPacket* pPacket)
{
	CPacket* pSendPacket;
	short    shType;
	__int64  iAccountNo;
	char     SessionKey[dfMAX_SESSIONKEY_LEN + 1];

	
	*pPacket >> shType >> iAccountNo;
	pPacket->GetData(SessionKey, dfMAX_SESSIONKEY_LEN);
	SessionKey[dfMAX_SESSIONKEY_LEN] = '\0';
	pPacket->SubRef();

	if (shType != en_PACKET_CS_LOGIN_REQ_LOGIN)
	{
		Disconnect(iSessionID);
	}

	pSendPacket = mpLoginResponse(iAccountNo, CheckAccountValid(iAccountNo, SessionKey));
	SendPacketInstant(iSessionID, pSendPacket, TRUE);

	InterlockedIncrement((long*)&iLoginTPS);
	pSendPacket->SubRef();
}

char CLoginServer::CheckAccountValid(__int64 iAccountNo, char* pSessionKey)
{
	bool ret_query;
	char Query[256];
	int  iRowCount;

	sprintf_s(Query, "SELECT * FROM `%s`.account WHERE accountno=%lld",
		DBName, iAccountNo);

	PRO_BEGIN("DB");

#ifndef _LOGIN_CONNECTIONPOOL
	DBConnector.Lock();
#endif

	ret_query = DBConnector.Execute(Query);
	if (ret_query != TRUE)
	{
		CRASH();
	}
	DBConnector.StoreResult();
	DBConnector.GetRowCount(iRowCount);
	if (iRowCount != 1)
	{
		return dfLOGIN_STATUS_ACCOUNT_MISS;
	}
	DBConnector.FreeResult();

#ifndef _LOGIN_CONNECTIONPOOL
	DBConnector.UnLock();
#endif
	PRO_END("DB");


	if (InsertToken(iAccountNo, pSessionKey) == FALSE)
	{
		return dfLOGIN_STATUS_FAIL;
	}

	return dfLOGIN_STATUS_OK;
}

bool CLoginServer::InsertToken(__int64 iAccountNo, char* pSessionKey)
{
	std::string AccountNo = std::to_string(iAccountNo);
	std::string SessionKey = pSessionKey;
	std::string GetValue;
	std::future<cpp_redis::reply> get_reply;
	cpp_redis::reply ret;

	PRO_BEGIN("Redis");
#ifndef _LOGIN_CONNECTIONPOOL
	AcquireSRWLockExclusive(&RedisLock);
#endif
	get_reply = RedisClient.setex(AccountNo, 40, SessionKey);
	RedisClient.sync_commit();

	ret = get_reply.get();
#ifndef _LOGIN_CONNECTIONPOOL
	ReleaseSRWLockExclusive(&RedisLock);
#endif
	PRO_END("Redis");
	if (ret.is_null())
	{
		return FALSE;
	}

	GetValue = ret.as_string();


	if (GetValue.compare("OK") == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


CPacket* CLoginServer::mpLoginResponse(__int64 iAccountNo, char chTokenCheck)
{
	CPacket* pSendPacket;
	WCHAR    buf[20];

	pSendPacket = CPacket::Alloc();
	if (pSendPacket == nullptr)
	{
		CRASH();
	}

	*pSendPacket << (short)en_PACKET_CS_LOGIN_RES_LOGIN;

	*pSendPacket << iAccountNo;
	*pSendPacket << chTokenCheck;

	pSendPacket->PutData((char*)buf, sizeof(WCHAR) * 20);
	pSendPacket->PutData((char*)buf, sizeof(WCHAR) * 20);

	pSendPacket->PutData((char*)GameServerIP, sizeof(WCHAR) * 16);
	*pSendPacket << GameServerPort;
	pSendPacket->PutData((char*)ChatServerIP, sizeof(WCHAR) * 16);
	*pSendPacket << ChatServerPort;
	
	return pSendPacket;
}


void CLoginServer::OnMonitoring(void)
{
	int    iTimeStamp;

	int    iCPUProcess;
	int    iProcessMem;
	int    iSession;

	int    iAccept;
	int    iRelease;

	int iRecv;
	int iRecvB;
	int iSend;
	int iSendB;

	int iLogin;

	int iPacketPoolSize;

	iTimeStamp = (int)time(NULL);
	HardwareStatus.UpdateHardwareStatus();

	iCPUProcess = HardwareStatus.ProcessTotal();
	iProcessMem = HardwareStatus.UsedMemoryMB();
	iSession = GetSessionCount();

	iAccept = GetAcceptTPS();
	iRelease = GetReleaseTPS();
	iLogin = InterlockedExchange((long*)&iLoginTPS, 0);

	iRecv = GetRecvTPS();
	iRecvB = GetRecvBytesTPS();
	iSend = GetSendTPS();
	iSendB = GetSendBytesTPS();

	iPacketPoolSize = CPacket::GetPacketPoolAllocSize();

	wprintf(L"===================================================================================\n");
	CLog::PrintRunTimeLog();
	HardwareStatus.PrintHardwareStatus();
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"Session Count : %d\n", GetSessionCount());
	wprintf(L"Accept  TPS    : %-8d(Total %d)\n", iAccept,  GetTotalAccept());
	wprintf(L"Release TPS    : %-8d(Total %d)\n", iRelease, GetTotalRelease());
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"Recv    TPS    : %-8d(%d bytes)\n", iRecv, iRecvB);
	wprintf(L"Send    TPS    : %-8d(%d bytes)\n", iSend, iSendB);
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"PacketPool     : %-8d / %-8d\n", CPacket::GetPacketPoolTotalSize() - iPacketPoolSize, CPacket::GetPacketPoolTotalSize());

	wprintf(L"===================================================================================\n\n");
	
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, TRUE, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, iCPUProcess, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, iProcessMem, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_LOGIN_SESSION, iSession, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, iLogin, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, iPacketPoolSize, iTimeStamp);
}