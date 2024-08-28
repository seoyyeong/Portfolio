#include "pch.h"
#include "CMonitoringClient.h"

CMonitoringClient::CMonitoringClient()
{
	chServerType = -1;
	bIsAlive = FALSE;
}
CMonitoringClient::~CMonitoringClient()
{

}

void CMonitoringClient::Initialize(const WCHAR* FileName)
{
	CParser Parse;

	Parse.LoadFile(FileName);

	Parse.GetValue(dfMONITORING_CLIENTNAME, L"SERVER_TYPE", &chServerType, 1);

	WCHAR ServerIP[20];
	short ServerPort;
	int iWorkerCnt;
	int iActiveWorkerCnt;

	Parse.GetValue(dfMONITORING_CLIENTNAME, L"SERVER_IP", ServerIP, 20);
	Parse.GetValue(dfMONITORING_CLIENTNAME, L"SERVER_PORT", ServerPort);

	Parse.GetValue(dfMONITORING_CLIENTNAME, L"IOCP_WORKER_THREAD", iWorkerCnt);
	Parse.GetValue(dfMONITORING_CLIENTNAME, L"IOCP_ACTIVE_THREAD", iActiveWorkerCnt);
	InitClient(ServerIP, ServerPort, iWorkerCnt, iActiveWorkerCnt);
}

bool CMonitoringClient::StartMonitoringClient(void)
{
	return StartClient();
}


void CMonitoringClient::StopMonitoringClient(void)
{
	StopClient();
}

void CMonitoringClient::TryConnect(void)
{
	PostConnect();
}

void CMonitoringClient::SendMonitoringPacket(char chType, int iValue, int iTimeStamp)
{
	CPacket* pPacket;

	if (bIsAlive == TRUE)
	{
		pPacket = mpMonitoringUpdate(chType, iValue, iTimeStamp);
		SendPacket(pPacket);
		pPacket->SubRef();
	}
}

void CMonitoringClient::OnServerConnect(short shPort)
{
	CPacket* pPacket;

	bIsAlive = TRUE;
	pPacket = mpMonitoringLogin(chServerType);
	SendPacket(pPacket);
	pPacket->SubRef();
}

void CMonitoringClient::OnServerLeave(void)
{
	bIsAlive = FALSE;
	TryConnect();
}

void CMonitoringClient::OnRecv(CPacket* pPacket)
{
	pPacket->AddRef();
	PacketProc(pPacket);
}

bool CMonitoringClient::PacketProc(CPacket* pPacket)
{
	char chType;
	char chResult;

	*pPacket >> chType;
	if (chType == chServerType)
	{
		*pPacket >> chResult;

		if (chType == TRUE)
		{
			bIsAlive = TRUE;
		}
	}
	else
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"Client PacketProc Error : Wrong ServerType");
		Disconnect();
	}

	pPacket->SubRef();

	return TRUE;
}

CPacket* CMonitoringClient::mpMonitoringLogin(char chServerType)
{
	CPacket* pPacket = CPacket::Alloc();
	if (pPacket == nullptr)
	{
		return nullptr;
	}

	*pPacket << (short)en_PACKET_SS_MONITOR_LOGIN << chServerType;

	return pPacket;
}

CPacket* CMonitoringClient::mpMonitoringUpdate(char chType, int iValue, int iTimeStamp)
{
	CPacket* pPacket = CPacket::Alloc();
	if (pPacket == nullptr)
	{
		return nullptr;
	}

	*pPacket << (short)en_PACKET_SS_MONITOR_DATA_UPDATE << chType << iValue << iTimeStamp;

	return pPacket;
}