#include "pch.h"
#include "CMonitoringServer.h"

CMonitoringServer Server;
CCrashDump  Dump;

int wmain(int* argc, WCHAR* argv[])
{

	Server.StartMonitoringServer(L"MonitoringServer.cnf");

	return 0;
}