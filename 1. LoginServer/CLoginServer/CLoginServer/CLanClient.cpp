#include "pch.h"
#include "CLanClient.h"


CLanClient::CLanClient()
{
	WSADATA wsa;

	timeBeginPeriod(1);
	srand(0);

	while (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		_LOG(L"ERROR", CLog::LEVEL_ERROR, L"####### WSA STARTUP ERROR\n");
	}
	bRunning = FALSE;
}

CLanClient::~CLanClient()
{
	if (bRunning)
	{
		StopClient();
	}

	WSACleanup();
}

void CLanClient::MonitoringFunc(void)
{
}

bool CLanClient::StartClient(void)
{
	if (InterlockedExchange8((char*)&bRunning, TRUE) == TRUE)
	{
		return FALSE;
	}

	ConnectToServer();

	//Start Thread
	InitThread();

	CLog::SetLevel(CLog::LEVEL_SYSTEM);

	bRunning = TRUE;

	MonitoringThread(nullptr);


	return TRUE;
}

void CLanClient::StopClient(void)
{

	Disconnect();

	for (int i = 0; i < iWorkerCnt; i++)
	{
		PostQueuedCompletionStatus(hIOCP, dfJOBTYPE_SHUTDOWN, 0, 0);
	}

	SetEvent(hExitEvent);
	WaitForMultipleObjects(iThreadCnt, &ThreadArr[0], TRUE, INFINITE);

	for (int i = 0; i < iThreadCnt; i++)
	{
		CloseHandle(ThreadArr[i]);
	}

	InterlockedExchange8((char*)&bRunning, FALSE);
}

void CLanClient::InitClient(const WCHAR* ServerIP, short ServerPort, int iWorker, int iActive)
{
	wcscpy_s(this->ServerIP, ServerIP);
	this->ServerPort = ServerPort;
	this->iWorkerCnt = iWorker;
	this->iActiveWorkerCnt = iActive;

	iSessionIdx = 0;


	hIOCP = NULL;

	//IOCP
	while (hIOCP == NULL)
	{
		hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, iActiveWorkerCnt);
	}


	iRecvTPS = 0;
	iSendTPS = 0;
	iRecvBytesTPS = 0;
	iSendBytesTPS = 0;
	iRecvMsg = 0;
	iSendMsg = 0;
}


bool CLanClient::ConnectToServer(void)
{

	if (ConnectFunc())
	{
		OnServerConnect(Session.shPort);

		RecvPost();

		DecreaseIOCount();
		return TRUE;
	}

	return FALSE;

}

void CLanClient::PostConnect(void)
{
	PostQueuedCompletionStatus(hIOCP, 1, (ULONG_PTR)&Session, (LPOVERLAPPED)dfJOBTYPE_CONNECT);
}

void CLanClient::InitThread(void)
{
	hExitEvent = (HANDLE)CreateEvent(NULL, TRUE, FALSE, NULL);

	//Worker
	for (int i = 0; i < iWorkerCnt; i++)
	{
		ThreadArr[iThreadCnt++] = (HANDLE)_beginthreadex(NULL, 0, WorkerExecuter, this, TRUE, NULL);
		iThreadCnt;
	}

}


bool CLanClient::Disconnect(void)
{
	if (Session.bIsAlive == FALSE)
	{
		return FALSE;
	}

	if ((IncreaseIOCount() & 0x80000000) != 0)
	{
		DecreaseIOCount();
		return FALSE;
	}

	if (Session.iSessionID != iSessionIdx)
	{
		DecreaseIOCount();
		return FALSE;
	}

	InterlockedExchange8((char*)&Session.bIsAlive, FALSE);
	CancelIoEx((HANDLE)Session.sock, nullptr);
	DecreaseIOCount();

	return TRUE;
}

bool CLanClient::SendPacket(CPacket* pPacket, bool bReserveRelease)
{
	if (Session.bIsAlive == FALSE)
	{
		return FALSE;
	}

	if ((IncreaseIOCount() & 0x80000000) != 0)
	{
		DecreaseIOCount();
		return FALSE;
	}

	if (Session.iSessionID != iSessionIdx)
	{
		DecreaseIOCount();
		return FALSE;
	}

	pPacket->AddRef();
	pPacket->Encode(dfHEADER_LAN);
	Session.SendQ.Enqueue(pPacket);

	if (bReserveRelease == TRUE)
	{
		Session.dwReservedRelease = GetTime() + dwReserveDisconnect;
	}

	PostQueuedCompletionStatus(hIOCP, 1, (ULONG_PTR)&Session, (LPOVERLAPPED)dfJOBTYPE_SEND);
	
	return TRUE;
}

bool CLanClient::ConnectFunc(void)
{
	linger  lin = { 1,0 };
	int     nagle = TRUE;
	int     iSendBuf = 0;

	u_long  on = 1;
	timeval timeout = { 5, 0 };
	FD_SET  wset;

	int     ret_conn;
	int     err_conn;
	HANDLE  err;


	if (SetSession() == FALSE)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"Connect : SetSession Error\n");
	}

	Session.sock = socket(AF_INET, SOCK_STREAM, 0);
	while (Session.sock == INVALID_SOCKET)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"####### CREATE SOCKET ERROR\n");
	}

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(ServerPort);
	InetPton(AF_INET, ServerIP, &(ServerAddr.sin_addr.S_un.S_addr));

	setsockopt(Session.sock, SOL_SOCKET, SO_LINGER, (char*)&lin, sizeof(linger));
	setsockopt(Session.sock, SOL_SOCKET, SO_SNDBUF, (char*)&iSendBuf, sizeof(iSendBuf));
	ioctlsocket(Session.sock, FIONBIO, &on);


	ret_conn = connect(Session.sock, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
	if (ret_conn == SOCKET_ERROR)
	{
		err_conn = WSAGetLastError();

		if (err_conn == WSAEWOULDBLOCK)
		{
			FD_ZERO(&wset);

			FD_SET(Session.sock, &wset);

			ret_conn = select(0, nullptr, &wset, nullptr, &timeout);

			if (!FD_ISSET(Session.sock, &wset))
			{
				err_conn = WSAGetLastError();

				_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"####### Connect ERROR %d\n", err_conn);
				DecreaseIOCount();

				return FALSE;
			}
		}
	}

	err = CreateIoCompletionPort((HANDLE)Session.sock, hIOCP, (ULONG_PTR)&Session, 0);
	if (err == NULL)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"Accept : Create IOCP Port Error %d", GetLastError());
		DecreaseIOCount();
		return FALSE;
	}

	return TRUE;
}

bool CLanClient::SetSession(void)
{
	if (Session.iSessionID != -1)
	{
		return FALSE;
	}

	Session.iSessionID = iSessionIdx;
	InterlockedAnd((long*)&Session.iIOCount, 0x7FFFFFFF);

	InterlockedExchange8((char*)&Session.bIsAlive, TRUE);
	InterlockedExchange8((char*)&Session.bSendFlag, FALSE);

	Session.dwLastSend = GetTime();
	Session.dwLastRecv = GetTime();
	Session.dwTimeOut = GetTime() + dwLoginTimeOut;

	return TRUE;
}

unsigned CLanClient::WorkerThread(void* lpParam)
{
	bool          ret;
	DWORD         cbTransferred;
	CSession*     pSession;
	stOverlapped* pOverlapped;


	while (1)
	{
		ret = FALSE;
		cbTransferred = 0;
		pSession = nullptr;
		pOverlapped = nullptr;

		ret = GetQueuedCompletionStatus(hIOCP, &cbTransferred, (PULONG_PTR)&pSession, (LPOVERLAPPED*)&pOverlapped, INFINITE);

		if (pOverlapped == nullptr)
		{
			if (cbTransferred != dfJOBTYPE_SHUTDOWN)
			{
				_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"Workerthread : GQCS ERROR %d", GetLastError());
			}
			return 0;
		}

		if (cbTransferred == 0)
		{
			Disconnect();
		}

		else
		{
			if (pOverlapped == &Session.stOverRecv)
			{
				RecvProc(cbTransferred);
			}
			if (pOverlapped == &Session.stOverSend)
			{
				SendProc(cbTransferred);
			}
			if (pOverlapped == (stOverlapped*)dfJOBTYPE_SEND)
			{
				SendPost();
			}
			if (pOverlapped == (stOverlapped*)dfJOBTYPE_CONNECT)
			{
				ConnectToServer();
				continue;
			}
		}

		DecreaseIOCount();
	}
}

unsigned CLanClient::MonitoringThread(void* lpParam)
{
	return 0;
}

bool CLanClient::Release(void)
{
	CPacket* pPacket;
	SOCKET   sock;
	__int64  iSessionID;

	if (InterlockedCompareExchange((long*)&Session.iIOCount, 0x80000001, 0) != 0)
	{
		return FALSE;
	}

	InterlockedExchange8((char*)&Session.bIsAlive, FALSE);

	sock = Session.sock;
	iSessionID = Session.iSessionID;

	Session.sock = INVALID_SOCKET;

	CancelIoEx((HANDLE)sock, nullptr);
	closesocket(sock);

	Session.RecvQ.ClearBuffer();

	for (int i = 0; i < Session.iSendPacket; i++)
	{
		Session.SendPacketArr[i]->SubRef();
		Session.SendPacketArr[i] = nullptr;
	}
	Session.iSendPacket = 0;

	while (Session.SendQ.Dequeue(pPacket))
	{
		pPacket->SubRef();
	}

	InterlockedExchange8((char*)&Session.bSendFlag, FALSE);
	InterlockedExchange64(&Session.iSessionID, -1);
	iSessionIdx++;
	OnServerLeave();

	return TRUE;
}

void CLanClient::RecvProc(DWORD cbTransferred)
{
	CPacket* pPacket;

	InterlockedIncrement((long*)&iRecvTPS);
	InterlockedAdd((long*)&iRecvBytesTPS, cbTransferred);

	Session.RecvQ.MoveRear(cbTransferred);
	Session.dwLastRecv = GetTime();

	while (1)
	{
		pPacket = CPacket::Alloc();
		if (RecvPacket(pPacket) == FALSE)
		{
			pPacket->SubRef();
			break;
		}

		InterlockedIncrement((long*)&iRecvMsg);
		OnRecv(pPacket);
		pPacket->SubRef();
	}

	RecvPost();
}

bool CLanClient::RecvPacket(CPacket* pPacket)
{
	stLanHeader* Header;

	if (Session.RecvQ.GetUsedSize() < dfHEADER_NET)
	{
		return FALSE;
	}
	
	Session.RecvQ.Peek((char*)pPacket->GetWritePtr(), dfHEADER_NET);
	Header = (stLanHeader*)pPacket->GetReadPtr();

	//네트워크 헤더 검증

	if (Header->shPayload <= 0 || Header->shPayload > dfMAX_MESSAGE_LEN * 2)
	{
		Disconnect();
		return FALSE;
	}

	if (Session.RecvQ.GetUsedSize() >= dfHEADER_LAN + Header->shPayload)
	{
		pPacket->MoveWritePos(dfHEADER_LAN);
		Session.RecvQ.MoveFront(dfHEADER_LAN);
		Session.RecvQ.Dequeue((char*)pPacket->GetWritePtr(), Header->shPayload);
		pPacket->MoveWritePos(Header->shPayload);

		pPacket->Decode(dfHEADER_LAN);
		return TRUE;
	}

	return FALSE;
}

void CLanClient::RecvPost(void)
{
	WSABUF  wsabuf[2];
	DWORD   dwRecvBytes;
	DWORD   flags;

	int     ret_recv;
	int     err;

	if (InterlockedOr8((char*)&Session.bIsAlive, FALSE) == FALSE)
	{
		return;
	}

	IncreaseIOCount();

	memset(&Session.stOverRecv.stOverlapped, 0, sizeof(OVERLAPPED));

	flags = 0;

	//wsabuf 셋팅
	if (Session.RecvQ.GetBufRear() >= Session.RecvQ.GetBufFront())
	{
		wsabuf[0].buf = Session.RecvQ.GetBufRear();
		wsabuf[0].len = Session.RecvQ.DirectEnqueueSize();
		wsabuf[1].buf = Session.RecvQ.GetBuf();
		wsabuf[1].len = Session.RecvQ.GetAvailableSize() - Session.RecvQ.DirectEnqueueSize();
		ret_recv = WSARecv(Session.sock, wsabuf, 2, &dwRecvBytes, &flags, &Session.stOverRecv.stOverlapped, NULL);
	}
	else
	{
		wsabuf[0].buf = Session.RecvQ.GetBufRear();
		wsabuf[0].len = Session.RecvQ.DirectEnqueueSize();
		ret_recv = WSARecv(Session.sock, wsabuf, 1, &dwRecvBytes, &flags, &Session.stOverRecv.stOverlapped, NULL);
	}

	if (ret_recv == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != 10053 && err != 10054)
			{
				_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"RecvPost Error : WSArecv error %d", err);
			}
			DecreaseIOCount();
		}
	}

}

void CLanClient::SendProc(DWORD cbTransferred)
{
	int iSendPacket;

	Session.dwLastSend = GetTime();
	iSendPacket = Session.iSendPacket;

	for (int i = 0; i < iSendPacket; i++)
	{
		Session.SendPacketArr[i]->SubRef();
		Session.SendPacketArr[i] = nullptr;
	}
	Session.iSendPacket = 0;
	InterlockedExchange8((char*)&Session.bSendFlag, FALSE);

	InterlockedAdd((long*)&iSendMsg, iSendPacket);
	InterlockedAdd((long*)&iSendBytesTPS, cbTransferred);

	SendPost();
}

bool CLanClient::SendPost(void)
{
	CPacket* pPacket;
	WSABUF   wsabuf[MAX_WSA];
	DWORD    dwSendBytes;
	DWORD    flags;

	int      iSendCnt;
	int      ret_send;
	int      err;

	if (InterlockedOr8((char*)&Session.bIsAlive, FALSE) == FALSE)
	{
		return FALSE;
	}

	if (InterlockedExchange8((char*)&Session.bSendFlag, TRUE) != FALSE)
	{
		return FALSE;
	}

	iSendCnt = Session.SendQ.Size();

	if (iSendCnt <= 0)
	{
		InterlockedExchange8((char*)&Session.bSendFlag, FALSE);
		return FALSE;
	}

	IncreaseIOCount();
	memset(&Session.stOverSend.stOverlapped, 0, sizeof(OVERLAPPED));

	Session.iSendPacket = min(MAX_WSA, iSendCnt);

	for (int i = 0; i < Session.iSendPacket; i++)
	{
		Session.SendQ.Dequeue(pPacket);
		Session.SendPacketArr[i] = pPacket;

		wsabuf[i].buf = pPacket->GetPtr();
		wsabuf[i].len = pPacket->GetSendSize();
	}

	flags = 0;

	ret_send = WSASend(Session.sock, wsabuf, Session.iSendPacket, &dwSendBytes, flags, &Session.stOverSend.stOverlapped, NULL);

	if (ret_send == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != 10053 && err != 10054)
			{
				_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"SendPost Error : wsasend error %d", err);
			}
			DecreaseIOCount();
			return FALSE;
		}
	}

	InterlockedIncrement((long*)&iSendTPS);

	return TRUE;
}
