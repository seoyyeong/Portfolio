#include "pch.h"
#include "CMonitoringCSServer.h"

CMonitoringCSServer::CMonitoringCSServer()
{
	InitializeSRWLock(&ClientLock);
	ClientPool = nullptr;
}

CMonitoringCSServer::~CMonitoringCSServer()
{
	if (bRunning)
	{
		StopMonitoringCSServer();
	}
	if (ClientPool != nullptr)
	{
		delete ClientPool;
	}
}

void CMonitoringCSServer::StartMonitoringCSServer(const WCHAR* FileName)
{
	Parse.LoadFile(FileName);

	InitMonitoringCSServer();
	StartServer();

}

void CMonitoringCSServer::StopMonitoringCSServer(void)
{
	StopServer();
}


void CMonitoringCSServer::InitMonitoringCSServer(void)
{
	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"CLIENT_MAX", iMaxClient);
	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"LOGIN_KEY", (char*)SessionKey, dfMONITORING_CS_KEYSIZE);
	ClientPool = new CObjectFreeList<stClient>(iMaxClient);

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

	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"PACKET_CODE", (char*)&chHeaderCode, 1);
	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"PACKET_KEY", (char*)&chHeaderKey, 1);
	CPacket::SetHeaderCode(chHeaderCode);
	CPacket::SetHeaderKey(chHeaderKey);

	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"BIND_IP", ServerIP, 20);
	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"BIND_PORT", ServerPort);
	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"NAGLE", iNagle);
	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"ZEROCOPYSEND", bZeroCopySend);

	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"IOCP_WORKER_THREAD", iWorkerCnt);
	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"IOCP_ACTIVE_THREAD", iActiveWorkerCnt);

	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"SESSION_MAX", iMaxSession);

	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"TIMEOUT_LOGIN", dwLoginTimeOut);
	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"TIMEOUT_SENDRESERVE", dwReserveDisconnect);

	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"CHECK_TIMEOUT", bCheckTimeOut);
	Parse.GetValue(dfMONITERING_CS_SERVERNAME, L"RESERVE_DISCONNECT", bReserveDisconnect);

	InitServer(ServerIP, ServerPort, iNagle, bZeroCopySend, iWorkerCnt, iActiveWorkerCnt, iMaxSession, dwLoginTimeOut, dwReserveDisconnect, bCheckTimeOut, bReserveDisconnect);

}

void CMonitoringCSServer::OnClientAccept(SESSION_ID iSessionID, short shPort)
{
	stClient* pClient;
	
	pClient = ClientPool->Alloc();
	pClient->iSessionID = iSessionID;
	pClient->bIsAlive = FALSE;

	ClientMapLock();
	ClientMap.insert(std::make_pair(iSessionID, pClient));
	ClientMapUnLock();
}

void CMonitoringCSServer::OnClientLeave(SESSION_ID iSessionID)
{
	stClient* pClient;
	pClient = FindClient(iSessionID);

	ClientMapLock();
	ClientMap.erase(iSessionID);
	ClientMapUnLock();

	pClient->bIsAlive = FALSE;
	pClient->iSessionID = -1;

	ClientPool->Free(pClient);
}

void CMonitoringCSServer::OnRecv(SESSION_ID iSessionID, CPacket* pPacket)
{
	pPacket->AddRef();
	PacketProc(iSessionID, pPacket);
}

void CMonitoringCSServer::PacketProc(SESSION_ID iSessionID, CPacket* pPacket)
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
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		PacketProcCSLogin(iSessionID, pPacket);
		break;

	default:
		_LOG(L"MONITERING_CS", CLog::LEVEL_ERROR, L"PacketProc : Code Error %hd(Session %lld)", shType, iSessionID);
		Disconnect(iSessionID);
		break;
	}

	pPacket->SubRef();

}

void CMonitoringCSServer::PacketProcCSLogin(SESSION_ID iSessionID, CPacket* pPacket)
{
	stClient* pClient;
	CPacket*  pSendPacket;
	char      RecvKey[dfMONITORING_CS_KEYSIZE];

	if (pPacket->GetDataSize() < dfMONITORING_CS_KEYSIZE)
	{
		return;
	}

	pClient = FindClient(iSessionID);
	if (pClient == nullptr)
	{
		return;
	}

	pPacket->GetData(RecvKey, dfMONITORING_CS_KEYSIZE);

	if (memcmp(RecvKey, SessionKey, dfMONITORING_CS_KEYSIZE) == 0)
	{
		pSendPacket = mpMonitoringCSLogin(dfMONITOR_TOOL_LOGIN_OK);
		pClient->bIsAlive = TRUE;
		SetTimeOut(pClient->iSessionID, dwCSTimeOut);
	}
	else
	{
		_LOG(L"MONITERING_CS", CLog::LEVEL_ERROR, L"PacketProcLogin : Key Error Session %lld", iSessionID);
		pSendPacket = mpMonitoringCSLogin(dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY);
	}

	if (pSendPacket == nullptr)
	{
		return;
	}

	SendPacket(pClient->iSessionID, pSendPacket);
	iSendTPS++;
	iSendBytesTPS += (pPacket->GetDataSize());

	pSendPacket->SubRef();
}

CPacket* CMonitoringCSServer::mpMonitoringCSLogin(char chStatus)
{
	CPacket* pSendPacket;

	pSendPacket = CPacket::Alloc();
	if (pSendPacket == nullptr)
	{
		return nullptr;
	}

	*pSendPacket << (short)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN << chStatus;

	return pSendPacket;
}

void CMonitoringCSServer::ClientMapLock(void)
{
	AcquireSRWLockExclusive(&ClientLock);
}

void CMonitoringCSServer::ClientMapUnLock(void)
{
	ReleaseSRWLockExclusive(&ClientLock);
}

CMonitoringCSServer::stClient* CMonitoringCSServer::FindClient(SESSION_ID iSessionID)
{
	std::unordered_map<SESSION_ID, stClient*>::iterator iter;
	stClient* pClient = nullptr;

	ClientMapLock();

	iter = ClientMap.find(iSessionID);
	if (iter != ClientMap.end())
	{
		pClient = iter->second;
	}

	ClientMapUnLock();

	return pClient;
}

void CMonitoringCSServer::SendPacketToAllClients(CPacket* pPacket)
{
	std::unordered_map<SESSION_ID, stClient*>::iterator iter;

	ClientMapLock();
	for (iter = ClientMap.begin(); iter != ClientMap.end(); iter++)
	{
		if (iter->second->bIsAlive == TRUE)
		{
			SendPacket(iter->second->iSessionID, pPacket);
			iSendTPS++;
			iSendBytesTPS += (pPacket->GetDataSize());
		}
	}
	ClientMapUnLock();
}
