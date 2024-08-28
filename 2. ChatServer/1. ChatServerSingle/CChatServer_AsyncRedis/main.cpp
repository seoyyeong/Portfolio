#include "pch.h"
#include "CChatServer.h"

CChatServer Server;
CCrashDump  Dump;

int wmain(int* argc, WCHAR* argv[])
{

	Server.StartChatServer(L"ChatServer.cnf");

	return 0;
}