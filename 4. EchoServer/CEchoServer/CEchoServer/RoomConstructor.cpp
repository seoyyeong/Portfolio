#include "pch.h"
#include "RoomConstructor.h"

//-----------------------------------Room ���� �Լ� ������ �迭


std::vector<RoomConstructor> RoomConstructors =
{
	CreateAuthRoom,

	CreateEchoRoom

};

//----------------------------------Room ���� �Լ�

CRoom* CreateAuthRoom(void* pParam, int iIdx, int iMaxPlayer, DWORD dwFrame)
{
	return (CRoom*)new CAuthRoom((CNetRoomServer*)pParam, iIdx, iMaxPlayer, dwFrame);
}

CRoom* CreateEchoRoom(void* pParam, int iIdx, int iMaxPlayer, DWORD dwFrame)
{
	return (CRoom*)new CEchoRoom((CNetRoomServer*)pParam, iIdx, iMaxPlayer, dwFrame);
}

