#include "pch.h"
#include "CLoginServer.h"

CLoginServer Server;
CCrashDump  Dump;

int wmain(int* argc, WCHAR* argv[])
{

	Server.StartLoginServer(L"LoginServer.cnf");

	return 0;
}