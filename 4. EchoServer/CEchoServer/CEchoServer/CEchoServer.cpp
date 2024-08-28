#include "pch.h"
#include "CEchoServer.h"

CEchoServer::CEchoServer()
{
	this->pRoomManager = CRoomManager::GetInstance();
}

CEchoServer::~CEchoServer(void)
{

}


void CEchoServer::Initialize(const WCHAR* FileName)
{
	CParser Parse;
	int iLogLevel;
	char chHeaderCode;
	char chHeaderKey;

	iMaxPlayer = 0;
	dwAuthSleep = 0;
	dwEchoSleep = 0;

	Parse.LoadFile(FileName);

	Parse.GetValue(dfECHOSERVER_NAME, L"USER_MAX", iMaxPlayer);
	Parse.GetValue(dfECHOSERVER_NAME, L"AUTH_SLEEP", dwAuthSleep);
	Parse.GetValue(dfECHOSERVER_NAME, L"ECHO_SLEEP", dwEchoSleep);
	Parse.GetValue(dfECHOSERVER_NAME, L"LOG_LEVEL", iLogLevel);

	Parse.GetValue(dfECHOSERVER_NAME, L"PACKET_CODE", (char*)&chHeaderCode, 1);
	Parse.GetValue(dfECHOSERVER_NAME, L"PACKET_KEY", (char*)&chHeaderKey, 1);


	CPacket::SetHeaderCode(chHeaderCode);
	CPacket::SetHeaderKey(chHeaderKey);

	CLog::SetLevel(iLogLevel);

	iAuthRoomID = pRoomManager->CreateRoom(en_ROOM_AUTH, this, iMaxPlayer, dwAuthSleep);
	iEchoRoomID = pRoomManager->CreateRoom(en_ROOM_ECHO, this, iMaxPlayer, dwEchoSleep);

	MonitorClient.Initialize(FileName);

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


	Parse.GetValue(dfECHOSERVER_NAME, L"BIND_IP", ServerIP, 20);
	Parse.GetValue(dfECHOSERVER_NAME, L"BIND_PORT", ServerPort);
	Parse.GetValue(dfECHOSERVER_NAME, L"NAGLE", iNagle);
	Parse.GetValue(dfECHOSERVER_NAME, L"ZEROCOPYSEND", bZeroCopySend);

	Parse.GetValue(dfECHOSERVER_NAME, L"IOCP_WORKER_THREAD", iWorkerCnt);
	Parse.GetValue(dfECHOSERVER_NAME, L"IOCP_ACTIVE_THREAD", iActiveWorkerCnt);

	Parse.GetValue(dfECHOSERVER_NAME, L"SESSION_MAX", iMaxSession);

	Parse.GetValue(dfECHOSERVER_NAME, L"TIMEOUT_LOGIN", dwLoginTimeOut);
	Parse.GetValue(dfECHOSERVER_NAME, L"TIMEOUT_SENDRESERVE", dwReserveDisconnect);

	Parse.GetValue(dfECHOSERVER_NAME, L"CHECK_TIMEOUT", bCheckTimeOut);
	Parse.GetValue(dfECHOSERVER_NAME, L"RESERVE_DISCONNECT", bReserveDisconnect);

	InitServer(ServerIP, ServerPort, iNagle, bZeroCopySend, iWorkerCnt, iActiveWorkerCnt, iMaxSession, dwLoginTimeOut, dwReserveDisconnect, bCheckTimeOut, bReserveDisconnect);

}

void CEchoServer::OnServerStart(void)
{
	pRoomManager->Start(iAuthRoomID);
	pRoomManager->Start(iEchoRoomID);
	MonitorClient.StartMonitoringClient();
}

void CEchoServer::OnServerClose(void)
{
	pRoomManager->Close(iAuthRoomID);
	pRoomManager->Close(iEchoRoomID);
	MonitorClient.StopMonitoringClient();
}

void CEchoServer::OnClientAccept(SESSION_ID iSessionID, short shPort)
{
	pRoomManager->InsertSession(iAuthRoomID, iSessionID);
}


void CEchoServer::OnTimeOut(SESSION_ID iSessionID)
{
	Disconnect(iSessionID);
}

void CEchoServer::OnMonitoring(void)
{
	CAuthRoomMonitoring AuthMonitor;
	CEchoRoomMonitoring EchoMonitor;

	int    iTimeStamp;

	int    iCPUProcess;
	int    iProcessMem;
	int    iSession;
	int    iAuthUser;
	int    iGameUser;

	int    iAccept;
	int    iRelease;

	int iRecv;
	int iRecvB;
	int iSend;
	int iSendB;

	int iLogin;
	int iEcho;

	int iAuthFPSCnt;
	int iGameFPSCnt;

	int    iPacketPoolSize;
	int    iAuthPoolSize;
	int    iAuthAllocSize;
	int    iEchoPoolSize;
	int    iEchoAllocSize;
	int    iJobPoolSize;

	iTimeStamp = (int)time(NULL);

	HardwareStatus.UpdateHardwareStatus();
	pRoomManager->GetMonitoring(iAuthRoomID, &AuthMonitor, TRUE);
	pRoomManager->GetMonitoring(iEchoRoomID, &EchoMonitor, TRUE);

	iCPUProcess = HardwareStatus.ProcessTotal();
	iProcessMem = HardwareStatus.UsedMemoryMB();
	iSession = GetSessionCount();
	iAuthUser = AuthMonitor.iSession;
	iGameUser = EchoMonitor.iSession;

	iAccept = GetAcceptTPS();
	iRelease = GetReleaseTPS();

	iRecv = GetRecvMsgTPS();
	iRecvB = GetRecvBytesTPS();
	iSend = GetSendMsgTPS();
	iSendB = GetSendBytesTPS();

	iLogin = AuthMonitor.iAuthTPS;
	iEcho = EchoMonitor.iEchoTPS;

	iAuthFPSCnt = AuthMonitor.iFPS;
	iGameFPSCnt = EchoMonitor.iFPS;

	iPacketPoolSize = CPacket::GetPacketPoolAllocSize();
	iJobPoolSize = JobPool.GetAllocSize();
	iAuthPoolSize = AuthMonitor.iMaxPool;
	iAuthAllocSize = AuthMonitor.iPool;
	iEchoPoolSize = EchoMonitor.iMaxPool;
	iEchoAllocSize = EchoMonitor.iPool;
	//iPlayerPoolSize = PlayerPool.GetAllocSize();

	wprintf(L"===================================================================================\n");
	CLog::PrintRunTimeLog();
	HardwareStatus.PrintHardwareStatus();
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"Session Count  : %d(Auth %d / Game %d)\n\n", iSession, iAuthUser, iGameUser);
	wprintf(L"Accept  TPS    : %-8d(Total %d)\n", iAccept, GetTotalAccept());
	wprintf(L"Release TPS    : %-8d(Total %d)\n", iRelease, GetTotalRelease());
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"Recv    TPS    : %-8d(Call %8d / %d bytes)\n", iRecv, GetRecvTPS(), iRecvB);
	wprintf(L"Send    TPS    : %-8d(Call %8d / %d bytes)\n", iSend, GetSendTPS(), iSendB);
	wprintf(L"-----------------------------------------------------------------------------------\n");
	//wprintf(L"Job     TPS    : %-8d\n", iJob);
	wprintf(L"Auth    TPS    : %-8d \n", iLogin);
	wprintf(L"Game    TPS    : %-8d\n", iEcho);
	wprintf(L"-----------------------------------------------------------------------------------\n");
	wprintf(L"Auth    FPS    : %-8d\n", iAuthFPSCnt);
	wprintf(L"Game    FPS    : %-8d\n", iGameFPSCnt);
	wprintf(L"PacketPool     : %-8d / %-8d\n", CPacket::GetPacketPoolTotalSize() - iPacketPoolSize, CPacket::GetPacketPoolTotalSize());
	wprintf(L"AuthPlayerPool : %-8d / %-8d\n", iAuthAllocSize, iAuthPoolSize);
	wprintf(L"EchoPlayerPool : %-8d / %-8d\n", iEchoAllocSize, iEchoPoolSize);
	//wprintf(L"PlayerPool     : %-8d / %-8d\n", PlayerPool.GetTotalSize() - iPlayerPoolSize, PlayerPool.GetTotalSize());
	wprintf(L"JobPool        : %-8d / %-8d\n", JobPool.GetTotalSize() - iJobPoolSize, JobPool.GetTotalSize());
	wprintf(L"===================================================================================\n\n");

	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, TRUE, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, iCPUProcess, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, iProcessMem, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_SESSION, iSession, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, iAuthUser, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, iGameUser, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, iAccept, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, iRecv, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, iSend, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, 0, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG, 0, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, iAuthFPSCnt, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, iGameFPSCnt, iTimeStamp);
	MonitorClient.SendMonitoringPacket((char)dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, iPacketPoolSize, iTimeStamp);
}