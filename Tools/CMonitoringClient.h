#pragma once
#include "CLanClient.h"
#include "MonitorProtocol.h"

#define dfMONITORING_CLIENTNAME L"MONITORING_CLIENT"

class CMonitoringClient :public CLanClient
{
public:
	
	CMonitoringClient();
	~CMonitoringClient();

	void Initialize(const WCHAR* FileName);
	bool StartMonitoringClient(void);
	void StopMonitoringClient(void);

	void TryConnect(void);
	void SendMonitoringPacket(char chType, int iValue, int iTimeStamp);

private:
	bool OnConnectionRequest(WCHAR* IP, short shPort) { return TRUE; }
	void OnServerConnect(short shPort);
	void OnServerLeave(void);

	void OnRecv(CPacket* pPacket);
	void OnSend(int iSendSize){}

	void OnError(int iErrorCode, WCHAR* wch){}

	bool PacketProc(CPacket* pPacket);

	CPacket* mpMonitoringLogin(char chServerType);
	CPacket* mpMonitoringUpdate(char chType, int iValue, int iTimeStamp);

	char chServerType;
	bool bIsAlive;

};