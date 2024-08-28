#include "pch.h"
#include "CEchoServer.h"


int main(void)
{
	CEchoServer Server;

	Server.Initialize(L"EchoServer.cnf");
	Server.StartServer(dfECHOSERVER_NAME);



	return 0;
}