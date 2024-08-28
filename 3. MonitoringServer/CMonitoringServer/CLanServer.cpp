#include "pch.h"
#include "CLanServer.h"



CLanServer::CLanServer()
{
	WSADATA wsa;

	timeBeginPeriod(1);
	srand(0);

	while (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		_LOG(L"ERROR", CLog::LEVEL_ERROR, L"####### WSA STARTUP ERROR\n");
	}

	bRunning = FALSE;
	SessionArr = nullptr;
	SessionOpenList = nullptr;
}


CLanServer::~CLanServer()
{
	if (bRunning)
	{
		StopServer();
	}

	if (SessionOpenList == nullptr)
	{
		delete SessionOpenList;
	}
	if (SessionArr == nullptr)
	{
		delete[] SessionArr;
	}

	WSACleanup();
}

void CLanServer::InitServer(const WCHAR* ServerIP, short ServerPort, int Nagle, bool ZeroCopy, int iWorker, int iActive, int MaxSession, DWORD dwLoginTimeOut, DWORD dwReserveDisconnect, bool bCheckTimeOut, bool bReserveDisconnect)
{
	wcscpy_s(this->ServerIP, ServerIP);
	this->ServerPort = ServerPort;
	this->iNagle = Nagle;
	this->bZeroCopySend = ZeroCopy;
	this->iWorkerCnt = iWorker;
	this->iActiveWorkerCnt = iActive;
	this->iMaxSession = MaxSession;
	this->dwLoginTimeOut = dwLoginTimeOut;
	this->dwReserveDisconnect = dwReserveDisconnect;
	this->bCheckTimeOut = bCheckTimeOut;
	this->bReserveDisconnect = bReserveDisconnect;
}

bool CLanServer::StartServer(void)
{
	if (InterlockedExchange8((char*)&bRunning, TRUE) == TRUE)
	{
		return FALSE;
	}

	//initialize variables
	InitVariable();

	//Session Init
	InitSession();

	//Listen Socket Initialize
	InitNetwork();

	//Start Thread
	InitThread();

	MonitoringThread(nullptr);

	return TRUE;

}

void CLanServer::StopServer(void)
{

	closesocket(ListenSocket);

	InterlockedExchange8((char*)&bRunning, FALSE);

	for (int i = 0; i < iMaxSession; i++)
	{
		if (SessionArr[i].iSessionID != -1)
		{
			Disconnect(SessionArr[i].iSessionID);
		}
	}

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

}



void CLanServer::InitVariable(void)
{

	iSessionID = 0;
	iSessionCount = 0;

	hIOCP = NULL;

	SessionOpenList = new CLockFreeStack<int>(iMaxSession);

	iRecvTPS = 0;
	iSendTPS = 0;
	iRecvBytesTPS = 0;
	iSendBytesTPS = 0;
	iRecvMsg = 0;
	iSendMsg = 0;
	iAcceptTPS = 0;
	iReleaseTPS = 0;
	iTotalAccept = 0;
	iTotalRelease = 0;
}

void CLanServer::InitSession(void)
{
	SessionArr = new CSession[iMaxSession];
	for (__int64 i = iMaxSession - 1; i >= 0; i--)
	{
		SessionArr[i].iSessionidx = i << dfSESSIONINDEX_SHIFT;
		SessionOpenList->Push(i);
	}

}


void CLanServer::InitNetwork(void)
{
	linger lin = { 1,0 };
	int iSendBuf = 0;

	//Socket
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	while (ListenSocket == INVALID_SOCKET)
	{
		_LOG(L"ERROR", CLog::LEVEL_ERROR, L"####### CREATE LISTEN SOCKET ERROR\n");
	}


	InetPtonW(AF_INET, ServerIP, &ServerAddr);
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(ServerPort);

	setsockopt(ListenSocket, SOL_SOCKET, SO_LINGER, (char*)&lin, sizeof(linger));
	if (bZeroCopySend == TRUE)
	{
		setsockopt(ListenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&iSendBuf, sizeof(iSendBuf));
	}
	setsockopt(ListenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&iNagle, sizeof(iNagle));

	int ret_bind = bind(ListenSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
	while (ListenSocket == INVALID_SOCKET)
	{
		_LOG(L"ERROR", CLog::LEVEL_ERROR, L"####### BIND LISTEN SOCKET ERROR\n");
	}

	int ret_listen = listen(ListenSocket, SOMAXCONN_HINT(iMaxSession));
	while (ret_listen == SOCKET_ERROR)
	{
		_LOG(L"ERROR", CLog::LEVEL_ERROR, L"####### LISTEN SOCKET ERROR\n");
	}

	//IOCP
	while (hIOCP == NULL)
	{
		hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, iActiveWorkerCnt);
	}
}

void CLanServer::InitThread(void)
{
	hExitEvent = (HANDLE)CreateEvent(NULL, TRUE, FALSE, NULL);

	//CheckDisconnect Thread
	if (bCheckTimeOut == TRUE || bReserveDisconnect == TRUE)
	{
		ThreadArr[iThreadCnt++] = (HANDLE)_beginthreadex(NULL, 0, CheckDisconnectExecuter, this, TRUE, NULL);
	}


	//Accept Thread
	ThreadArr[iThreadCnt++] = (HANDLE)_beginthreadex(NULL, 0, AcceptExecuter, this, TRUE, NULL);


	//Worker
	for (int i = 0; i < iWorkerCnt; i++)
	{

		ThreadArr[iThreadCnt++] = (HANDLE)_beginthreadex(NULL, 0, WorkerExecuter, this, TRUE, NULL);
		iThreadCnt;
	}

}

bool CLanServer::SetTimeOut(SESSION_ID iSessionID, DWORD dwTime)
{
	CSession* pSession;
	pSession = FindSession(iSessionID);

	if (pSession == nullptr)
	{
		return FALSE;
	}

	if ((IncreaseIOCount(pSession) & 0x80000000) != 0)
	{
		DecreaseIOCount(pSession);
		return FALSE;
	}

	if (pSession->iSessionID != iSessionID)
	{
		DecreaseIOCount(pSession);
		return FALSE;
	}

	pSession->dwTimeOut = GetTime() + dwTime;

	DecreaseIOCount(pSession);

	return TRUE;
}

bool CLanServer::Disconnect(SESSION_ID iSessionID, short shType)
{
	CSession* pSession;
	pSession = FindSession(iSessionID);

	if (pSession == nullptr)
	{
		return FALSE;
	}

	if ((IncreaseIOCount(pSession) & 0x80000000) != 0)
	{
		DecreaseIOCount(pSession);
		return FALSE;
	}

	if (pSession->iSessionID != iSessionID)
	{
		DecreaseIOCount(pSession);
		return FALSE;
	}

	InterlockedExchange8((char*)&pSession->bIsAlive, FALSE);
	CancelIoEx((HANDLE)pSession->sock, nullptr);

	DecreaseIOCount(pSession);

	return TRUE;
}

bool CLanServer::SendPacket(SESSION_ID iSessionID, CPacket* pPacket, bool bReserveRelease)
{
	CSession* pSession;

	pSession = FindSession(iSessionID);
	if (pSession == nullptr)
	{
		return FALSE;
	}

	if (InterlockedOr8((char*)&pSession->bIsAlive, FALSE) == FALSE)
	{
		return FALSE;
	}

	if ((IncreaseIOCount(pSession) & 0x80000000) != 0)
	{
		DecreaseIOCount(pSession);
		return FALSE;
	}

	if (pSession->iSessionID != iSessionID)
	{
		DecreaseIOCount(pSession);
		return FALSE;
	}

	pPacket->AddRef();
	pPacket->Encode(dfHEADER_NET);
	pSession->SendQ.Enqueue(pPacket);

	PostQueuedCompletionStatus(hIOCP, 1, (ULONG_PTR)pSession, (LPOVERLAPPED)dfJOBTYPE_SEND);

	if (bReserveRelease == TRUE)
	{
		pSession->dwReservedRelease = GetTime() + dwReserveDisconnect;
		DisconnectQueue.Enqueue(pSession->iSessionID);
	}

	return TRUE;
}

bool CLanServer::SendPackets(SESSION_ID iSessionID, CPacket** pPacketArr, int iCount, bool bReserveRelease)
{
	CSession* pSession;
	CPacket* pPacket;

	pSession = FindSession(iSessionID);
	if (pSession == nullptr)
	{
		return FALSE;
	}

	if (InterlockedOr8((char*)&pSession->bIsAlive, FALSE) == FALSE)
	{
		return FALSE;
	}

	if ((IncreaseIOCount(pSession) & 0x80000000) != 0)
	{
		DecreaseIOCount(pSession);
		return FALSE;
	}

	if (pSession->iSessionID != iSessionID)
	{
		DecreaseIOCount(pSession);
		return FALSE;
	}

	for (int i = 0; i < iCount; i++)
	{
		pPacket = pPacketArr[i];
		pPacket->AddRef();
		pPacket->Encode(dfHEADER_NET);
		pSession->SendQ.Enqueue(pPacket);
	}

	PostQueuedCompletionStatus(hIOCP, 1, (ULONG_PTR)pSession, (LPOVERLAPPED)dfJOBTYPE_SEND);

	if (bReserveRelease == TRUE)
	{
		pSession->dwReservedRelease = GetTime() + dwReserveDisconnect;
		DisconnectQueue.Enqueue(pSession->iSessionID);
	}

	return TRUE;
}

bool CLanServer::SendPacketInstant(SESSION_ID iSessionID, CPacket* pPacket, bool bReserveRelease)
{
	CSession* pSession;

	pSession = FindSession(iSessionID);
	if (pSession == nullptr)
	{
		return FALSE;
	}

	if (InterlockedOr8((char*)&pSession->bIsAlive, FALSE) == FALSE)
	{
		return FALSE;
	}

	if ((IncreaseIOCount(pSession) & 0x80000000) != 0)
	{
		DecreaseIOCount(pSession);
		return FALSE;
	}

	if (pSession->iSessionID != iSessionID)
	{
		DecreaseIOCount(pSession);
		return FALSE;
	}

	pPacket->AddRef();
	pPacket->Encode(dfHEADER_NET);
	pSession->SendQ.Enqueue(pPacket);

	SendPost(pSession);

	if (bReserveRelease == TRUE)
	{
		pSession->dwReservedRelease = GetTime() + dwReserveDisconnect;
		DisconnectQueue.Enqueue(pSession->iSessionID);
	}
	DecreaseIOCount(pSession);

	return TRUE;
}


CSession* CLanServer::SetSession(SOCKET ClientSock, SOCKADDR_IN* ClientAddr)
{
	CSession* pSession;
	int        iSessionIdx;
	__int64    iNewSessionID;
	int        ret;
	HANDLE     err;

	ret = SessionOpenList->Pop(iSessionIdx);
	if (ret == FALSE)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"Accept : FULL SESSION");
		return nullptr;
	}

	pSession = &SessionArr[iSessionIdx];
	if (pSession->iSessionID != -1)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"Accept : Alive Session %d", pSession->iSessionID);
		CRASH();
	}
	iNewSessionID = (++iSessionID) | (pSession->iSessionidx);


	InterlockedAnd((long*)&pSession->iIOCount, 0x7FFFFFFF);

	InterlockedExchange8((char*)&pSession->bIsAlive, TRUE);
	InterlockedExchange8((char*)&pSession->bSendFlag, FALSE);

	pSession->sock = ClientSock;
	pSession->iSessionID = iNewSessionID;
	pSession->shPort = ntohs(ClientAddr->sin_port);
	pSession->lIP = ntohl(ClientAddr->sin_addr.S_un.S_addr);
	pSession->dwTimeOut = GetTime() + dwLoginTimeOut;

	err = CreateIoCompletionPort((HANDLE)ClientSock, hIOCP, (ULONG_PTR)pSession, 0);
	if (err == NULL)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"Accept : Create IOCP Port Error %d", GetLastError());
		return nullptr;
	}

	return pSession;
}

unsigned CLanServer::AcceptThread(void* lpParam)
{
	SOCKET      ClientSock;
	SOCKADDR_IN ClientAddr;
	int         iAddrlen;

	CSession* pSession;

	iAddrlen = sizeof(ClientAddr);

	while (1)
	{
		ClientSock = accept(ListenSocket, (sockaddr*)&ClientAddr, &iAddrlen);

		if (ClientSock == INVALID_SOCKET)
		{
			_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"Accept : listen socket Error : %d", WSAGetLastError());
			return 0;
		}

		pSession = SetSession(ClientSock, &ClientAddr);
		if (pSession == nullptr)
		{
			closesocket(ClientSock);
			continue;
		}

		iAcceptTPS++;
		iTotalAccept++;
		InterlockedIncrement((long*)&iSessionCount);

		OnClientAccept(pSession->iSessionID, pSession->shPort);

		RecvPost(pSession);

		DecreaseIOCount(pSession);
	}
}

unsigned CLanServer::WorkerThread(void* lpParam)
{
	bool          ret;
	DWORD         cbTransferred;
	CSession* pSession;
	stOverlapped* pOverlapped;


	while (1)
	{
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
			Disconnect(pSession->iSessionID);
		}

		else
		{
			if (pOverlapped == &pSession->stOverRecv)
			{
				RecvProc(pSession, cbTransferred);
			}
			else if (pOverlapped == &pSession->stOverSend)
			{
				SendProc(pSession, cbTransferred);
			}
			else if (pOverlapped == (stOverlapped*)dfJOBTYPE_SEND)
			{
				SendPost(pSession);
			}
		}

		DecreaseIOCount(pSession);
	}
}


unsigned CLanServer::CheckDisconnectThread(void* lpParam)
{
	SESSION_ID iReservedID = -1;
	SESSION_ID iTimeOutID;
	CSession* pSession;

	while (bRunning == TRUE)
	{
		Sleep(dwReserveDisconnect);

		//1. timeout check
		if (bCheckTimeOut == TRUE)
		{
			for (int i = 0; i < iMaxSession; i++)
			{
				iTimeOutID = SessionArr[i].iSessionID;
				if (SessionArr[i].bIsAlive == TRUE)
				{
					if (GetTime() > SessionArr[i].dwTimeOut)
					{
						OnTimeOut(iTimeOutID);
					}
				}
			}
		}

		//2. reserved disconnect check

		Disconnect(iReservedID);

		while (DisconnectQueue.Size() > 0)
		{
			DisconnectQueue.Dequeue(iReservedID);
			pSession = FindSession(iReservedID);
			if (pSession == nullptr) //해당 세션id가 없을 경우
			{
				continue;
			}
			if (GetTime() > pSession->dwReservedRelease) //disconnect 대상일 경우
			{
				Disconnect(iReservedID);
			}
			else //아직 disconnect 시간이 되지 않았을 경우
			{
				break;
			}

		}
	}

	return 0;
}


bool CLanServer::Release(CSession* pSession)
{
	CPacket* pPacket;
	SOCKET     sock;
	SESSION_ID iSessionID;

	if (InterlockedCompareExchange((long*)&pSession->iIOCount, 0x80000001, 0) != 0)
	{
		return FALSE;
	}

	InterlockedExchange8((char*)&pSession->bIsAlive, FALSE);

	sock = pSession->sock;
	iSessionID = pSession->iSessionID;

	pSession->iSessionID = -1;
	pSession->sock = INVALID_SOCKET;

	CancelIoEx((HANDLE)sock, nullptr);
	closesocket(sock);

	pSession->RecvQ.ClearBuffer();

	for (int i = 0; i < pSession->iSendPacket; i++)
	{
		pSession->SendPacketArr[i]->SubRef();
		pSession->SendPacketArr[i] = nullptr;
	}
	pSession->iSendPacket = 0;

	while (pSession->SendQ.Dequeue(pPacket))
	{
		pPacket->SubRef();
	}

	InterlockedExchange8((char*)&pSession->bSendFlag, FALSE);

	InterlockedIncrement((long*)&iReleaseTPS);
	InterlockedDecrement((long*)&iSessionCount);
	InterlockedIncrement((long*)&iTotalRelease);

	OnClientLeave(iSessionID);

	SessionOpenList->Push(pSession->iSessionidx >> dfSESSIONINDEX_SHIFT);

	return TRUE;
}

void CLanServer::RecvProc(CSession* pSession, DWORD cbTransferred)
{
	InterlockedIncrement((long*)&iRecvTPS);
	InterlockedAdd((long*)&iRecvBytesTPS, cbTransferred);

	pSession->dwLastRecv = GetTime();
	pSession->RecvQ.MoveRear(cbTransferred);

	RecvPacket(pSession);

	RecvPost(pSession);
}

void CLanServer::RecvPacket(CSession* pSession)
{
	CPacket* pPacket;

	while (1)
	{
		pPacket = CPacket::Alloc();
		if (CheckPacket(pSession, pPacket) == FALSE)
		{
			pPacket->SubRef();
			break;
		}

		InterlockedIncrement((long*)&iRecvMsg);
		OnRecv(pSession->iSessionID, pPacket);
		pPacket->SubRef();
	}
}

bool CLanServer::CheckPacket(CSession* pSession, CPacket* pPacket)
{
	stLanHeader* Header;

	if (pSession->RecvQ.GetUsedSize() < dfHEADER_LAN)
	{
		return FALSE;
	}

	pSession->RecvQ.Peek((char*)pPacket->GetWritePtr(), dfHEADER_LAN);
	Header = (stLanHeader*)pPacket->GetReadPtr();


	if (Header->shPayload <= 0 || Header->shPayload > dfMAX_MESSAGE_LEN * 2)
	{
		Disconnect(pSession->iSessionID);
		return FALSE;
	}

	if (pSession->RecvQ.GetUsedSize() >= dfHEADER_LAN + Header->shPayload)
	{
		pPacket->MoveWritePos(dfHEADER_LAN);
		pSession->RecvQ.MoveFront(dfHEADER_LAN);
		pSession->RecvQ.Dequeue((char*)pPacket->GetWritePtr(), Header->shPayload);
		pPacket->MoveWritePos(Header->shPayload);

		pPacket->Decode(dfHEADER_LAN);
		return TRUE;
	}

	return FALSE;
}

void CLanServer::RecvPost(CSession* pSession)
{
	WSABUF  wsabuf[2];
	DWORD   dwRecvBytes;
	DWORD   flags;

	int     ret_recv;
	int     err;

	if (InterlockedOr8((char*)&pSession->bIsAlive, FALSE) == FALSE)
	{
		return;
	}

	IncreaseIOCount(pSession);

	memset(&pSession->stOverRecv.stOverlapped, 0, sizeof(OVERLAPPED));

	flags = 0;

	//wsabuf 셋팅
	if (pSession->RecvQ.GetBufRear() >= pSession->RecvQ.GetBufFront())
	{
		wsabuf[0].buf = pSession->RecvQ.GetBufRear();
		wsabuf[0].len = pSession->RecvQ.DirectEnqueueSize();
		wsabuf[1].buf = pSession->RecvQ.GetBuf();
		wsabuf[1].len = pSession->RecvQ.GetAvailableSize() - pSession->RecvQ.DirectEnqueueSize();
		ret_recv = WSARecv(pSession->sock, wsabuf, 2, &dwRecvBytes, &flags, &pSession->stOverRecv.stOverlapped, NULL);
	}
	else
	{
		wsabuf[0].buf = pSession->RecvQ.GetBufRear();
		wsabuf[0].len = pSession->RecvQ.DirectEnqueueSize();
		ret_recv = WSARecv(pSession->sock, wsabuf, 1, &dwRecvBytes, &flags, &pSession->stOverRecv.stOverlapped, NULL);
	}

	if (ret_recv == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != 10004 && err != 10053 && err != 10054)
			{
				_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"RecvPost Error : WSArecv error %d(id %lld)", err, pSession->iSessionID);
			}
			DecreaseIOCount(pSession);
		}
	}

}

void CLanServer::SendProc(CSession* pSession, DWORD cbTransferred)
{
	int iSendPacket;

	pSession->dwLastSend = GetTime();
	iSendPacket = pSession->iSendPacket;

	for (int i = 0; i < iSendPacket; i++)
	{
		pSession->SendPacketArr[i]->SubRef();
	}
	pSession->iSendPacket = 0;
	InterlockedExchange8((char*)&pSession->bSendFlag, FALSE);

	InterlockedAdd((long*)&iSendMsg, iSendPacket);
	InterlockedAdd((long*)&iSendBytesTPS, cbTransferred);

	SendPost(pSession);
}

bool CLanServer::SendPost(CSession* pSession)
{
	CPacket* pPacket;
	WSABUF   wsabuf[dfLANSERVER_MAX_WSA];
	DWORD    dwSendBytes;
	DWORD    flags;

	int      iSendCnt;
	int      ret_send;
	int      err;

	if (InterlockedOr8((char*)&pSession->bIsAlive, FALSE) == FALSE)
	{
		return FALSE;
	}

	if (InterlockedExchange8((char*)&pSession->bSendFlag, TRUE) != FALSE)
	{
		return FALSE;
	}

	iSendCnt = pSession->SendQ.Size();

	if (iSendCnt <= 0)
	{
		InterlockedExchange8((char*)&pSession->bSendFlag, FALSE);
		return FALSE;
	}

	IncreaseIOCount(pSession);
	memset(&pSession->stOverSend.stOverlapped, 0, sizeof(OVERLAPPED));

	pSession->iSendPacket = min(dfLANSERVER_MAX_WSA, iSendCnt);

	for (int i = 0; i < pSession->iSendPacket; i++)
	{
		if (pSession->SendQ.Dequeue(pPacket) == FALSE)
		{
			CRASH();
		}
		pSession->SendPacketArr[i] = pPacket;

		wsabuf[i].buf = pPacket->GetPtr();
		wsabuf[i].len = pPacket->GetSendSize();
	}

	flags = 0;

	ret_send = WSASend(pSession->sock, wsabuf, pSession->iSendPacket, &dwSendBytes, flags, &pSession->stOverSend.stOverlapped, NULL);

	if (ret_send == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != 10004 && err != 10053 && err != 10054)
			{
				_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"SendPost Error : wsasend error %d(id %lld)", err, pSession->iSessionID);
			}
			DecreaseIOCount(pSession);
			return FALSE;
		}
	}

	InterlockedIncrement((long*)&iSendTPS);

	return TRUE;
}

unsigned CLanServer::MonitoringThread(void* lpParam)
{
	bool  bControl = FALSE;
	CFrame Frame(1000);

	while (1)
	{
		Frame.FrameControl();

		if (_kbhit())
		{
			char c = _getch();

			if (c == 'l' || c == 'L')
			{
				if (bControl == FALSE)
				{
					bControl = TRUE;
					wprintf(L"\n##### KEYBOARD CONTROL UNLOCKED #####\n\n");
				}
				else
				{
					bControl = FALSE;
					wprintf(L"\n##### KEYBOARD CONTROL LOCKED#####\n\n");
				}
			}

			if (bControl == TRUE)
			{

				if (c == 'q' || c == 'Q')
				{
					OnServerClose();
					StopServer();
					return 0;
				}
				if (c == 'z' || c == 'Z')
				{
					CRASH();
					return 0;
				}

				if (c == '0')
				{
					CLog::SetLevel(CLog::LEVEL_DEBUG);
				}
				if (c == '1')
				{
					CLog::SetLevel(CLog::LEVEL_WARNING);
				}
				if (c == '2')
				{
					CLog::SetLevel(CLog::LEVEL_ERROR);
				}
				if (c == '3')
				{
					CLog::SetLevel(CLog::LEVEL_SYSTEM);
				}
				else
				{
					OnServerControl(c);
				}
			}
		}

		if (bRunning)
		{
			OnMonitoring();
			RefreshTPS();
		}

	}
}