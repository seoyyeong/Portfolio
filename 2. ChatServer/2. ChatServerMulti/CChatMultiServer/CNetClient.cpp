
#include "CNetClient.h"


CNetClient::CNetClient()
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

CNetClient::~CNetClient()
{
	if (bRunning)
	{
		StopClient();
	}

	WSACleanup();
}

void CNetClient::MonitoringFunc(void)
{
}

bool CNetClient::StartClient(const WCHAR* ClientName)
{
	if (InterlockedExchange8((char*)&bRunning, TRUE) == TRUE)
	{
		return FALSE;
	}

	//initialize variables
	InitVariable(ClientName);

	//Listen Socket Initialize
	InitNetwork();

	//Start Thread
	InitThread();


	CLog::SetLevel(CLog::LEVEL_SYSTEM);

	bRunning = TRUE;

	MonitoringThread(nullptr);


	return TRUE;

}

void CNetClient::StopClient(void)
{
	//TODO : flag 추가
	//Session Clear

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



void CNetClient::InitVariable(const WCHAR* ClientName)
{
	//TODO:파일 읽어와서 수정...지금은 일단 그냥 타자로 가기

	iSessionID = 0;
	iSessionID = 0;
	iSessionCount = 0;

	Parse.GetValue(ClientName, L"BIND_IP", ServerIP, 20);
	Parse.GetValue(ClientName, L"BIND_PORT", ServerPort);

	Parse.GetValue(ClientName, L"IOCP_WORKER_THREAD", iWorkerCnt);
	Parse.GetValue(ClientName, L"IOCP_ACTIVE_THREAD", iActiveWorkerCnt);

	Parse.GetValue(ClientName, L"LOG_LEVEL", LogLevel, 10);

	Parse.GetValue(ClientName, L"PACKET_CODE", (char*)&chHeaderCode, 1);
	Parse.GetValue(ClientName, L"PACKET_KEY", (char*)&chHeaderCode, 1);

	Parse.GetValue(ClientName, L"TIMEOUT_LOGIN", dwTimeOut);
	Parse.GetValue(ClientName, L"TIMEOUT_SENDRESERVE", dwReserveDisconnect);

	CPacket::SetHeaderCode(chHeaderCode);
	CPacket::SetHeaderKey(chHeaderKey);

	hIOCP = NULL;

	iRecvCall = 0;
	iSendCall = 0;
	iRecvBytes = 0;
	iSendBytes = 0;
	iRecvTPS = 0;
	iSendTPS = 0;
	iRecvBytesTPS = 0;
	iSendBytesTPS = 0;
	iSendMsg = 0;

}

void CNetClient::InitNetwork(void)
{

	//IOCP
	while (hIOCP == NULL)
	{
		hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, iActiveWorkerCnt); //todo 나중에 조정 스레드개수
	}

	SetSession();

	OnClientConnect(Session.shPort);

	RecvPost();

	DecreaseIOCount();
}

void CNetClient::InitThread(void)
{
	hExitEvent = (HANDLE)CreateEvent(NULL, TRUE, FALSE, NULL);

	//Worker
	for (int i = 0; i < iWorkerCnt; i++)
	{
		ThreadArr[iThreadCnt++] = (HANDLE)_beginthreadex(NULL, 0, WorkerExecuter, this, TRUE, NULL);
		iThreadCnt;
	}

}


bool CNetClient::Disconnect(void)
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

	if (Session.iSessionID != iSessionID)
	{
		DecreaseIOCount();
		return FALSE;
	}

	InterlockedExchange8((char*)&Session.bIsAlive, FALSE);

	DecreaseIOCount();

	return TRUE;
}

bool CNetClient::SendPacket(CPacket* pPacket, bool bReserveRelease)
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

	if (Session.iSessionID != iSessionID)
	{
		DecreaseIOCount();
		return FALSE;
	}

	pPacket->AddRef();
	pPacket->Encode(dfHEADER_NET);
	Session.SendQ.Enqueue(pPacket);

	if (bReserveRelease == TRUE)
	{
		Session.dwReservedRelease = GetTime() + dwReserveDisconnect;
	}

	PostQueuedCompletionStatus(hIOCP, 1, (ULONG_PTR)&Session, (LPOVERLAPPED)dfJOBTYPE_SEND);
	
	return TRUE;
}

bool CNetClient::ConnectFunc(void)
{
	linger lin = { 1,0 };
	int    nagle = TRUE;
	int    iSendBuf = 0;
	int    ret_conn;
	HANDLE err;

	Session.sock = socket(AF_INET, SOCK_STREAM, 0);
	while (Session.sock == INVALID_SOCKET)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"####### CREATE SOCKET ERROR\n");
	}

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(ServerPort);
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	setsockopt(ListenSocket, SOL_SOCKET, SO_LINGER, (char*)&lin, sizeof(linger));
	setsockopt(ListenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&iSendBuf, sizeof(iSendBuf));
	//setsockopt(ListenSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&nagle, sizeof(nagle));

	ret_conn = connect(Session.sock, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr));
	if (ret_conn == SOCKET_ERROR)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"####### Connect ERROR %d\n", WSAGetLastError());
		return FALSE;
	}

	err = CreateIoCompletionPort((HANDLE)Session.sock, hIOCP, (ULONG_PTR)&Session, 0);
	if (err == NULL)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"Accept : Create IOCP Port Error %d", GetLastError());
		return FALSE;
	}

	return TRUE;
}

void CNetClient::SetSession(void)
{
	__int64    iNewSessionID;

	iNewSessionID = (++iSessionID) | (Session.iSessionidx);

	InterlockedAnd((long*)&Session.iIOCount, 0x7FFFFFFF);

	InterlockedExchange8((char*)&Session.bIsAlive, TRUE);
	InterlockedExchange8((char*)&Session.bSendFlag, FALSE);


	Session.iSessionID = iNewSessionID;
	Session.dwLastSend = GetTime();
	Session.dwLastRecv = GetTime();
	Session.dwTimeOut = GetTime() + dwTimeOut;
}

unsigned CNetClient::WorkerThread(void* lpParam)
{
	bool          ret;
	DWORD         cbTransferred;
	stSession* pSession;
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
		}

		DecreaseIOCount();
	}
}

unsigned CNetClient::MonitoringThread(void* lpParam)
{
	return 0;
}

bool CNetClient::Release(void)
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

	Session.iSessionID = -1;
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
	
	OnClientLeave();

	return TRUE;
}

void CNetClient::RecvProc(DWORD cbTransferred)
{
	CPacket* pPacket;

	InterlockedIncrement64(&iRecvCall);
	InterlockedAdd64(&iRecvBytes, cbTransferred);
	InterlockedAdd64(&iRecvBytesTPS, cbTransferred);

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

		InterlockedIncrement64(&iRecvTPS);
		OnRecv(pPacket);
		pPacket->SubRef();
	}

	RecvPost();
}

bool CNetClient::RecvPacket(CPacket* pPacket)
{
	stNetHeader* Header;

	if (Session.RecvQ.GetUsedSize() < dfHEADER_NET)
	{
		return FALSE;
	}
	
	Session.RecvQ.Peek((char*)pPacket->GetWritePtr(), dfHEADER_NET);
	Header = (stNetHeader*)pPacket->GetReadPtr();

	//네트워크 헤더 검증
	if (Header->chCode != dfHEADER_CODE)
	{
		_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"RecvPacket Error : Wrong Headercode");
		Disconnect();
		return FALSE;
	}

	if (Header->shLen <= 0 || Header->shLen > dfMAX_MESSAGE_LEN * 2)
	{
		Disconnect();
		return FALSE;
	}

	if (Session.RecvQ.GetUsedSize() >= dfHEADER_NET + Header->shLen)
	{
		pPacket->MoveWritePos(dfHEADER_NET);
		Session.RecvQ.MoveFront(dfHEADER_NET);
		Session.RecvQ.Dequeue((char*)pPacket->GetWritePtr(), Header->shLen);
		pPacket->MoveWritePos(Header->shLen);

		pPacket->Decode(dfHEADER_NET);
		return TRUE;
	}

	return FALSE;
}

void CNetClient::RecvPost(void)
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

void CNetClient::SendProc(DWORD cbTransferred)
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

	InterlockedIncrement64(&iSendCall);
	InterlockedAdd64(&iSendMsg, iSendPacket);
	InterlockedAdd64(&iSendBytes, cbTransferred);
	InterlockedAdd64(&iSendBytesTPS, cbTransferred);

	SendPost();
}

bool CNetClient::SendPost(void)
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
				_LOG(L"NETWORK", CLog::LEVEL_ERROR, L"SendPost Error : wsasend error %d(id %lld)", err, Session.iSessionID);
			}
			DecreaseIOCount();
			return FALSE;
		}
	}

	InterlockedIncrement((long*)&iSendTPS);

	return TRUE;
}
