#include "pch.h"
#include "RoomConstructor.h"

//-----------------------------------Room 생성 함수 포인터 배열


std::vector<RoomConstructor> RoomConstructors =
{
	CreateAuthRoom,

	CreateEchoRoom

};

//----------------------------------Room 생성 함수

CRoom* CreateAuthRoom(void* pParam, int iIdx, int iMaxPlayer, DWORD dwFrame)
{
	return (CRoom*)new CAuthRoom((CNetRoomServer*)pParam, iIdx, iMaxPlayer, dwFrame);
}

CRoom* CreateEchoRoom(void* pParam, int iIdx, int iMaxPlayer, DWORD dwFrame)
{
	return (CRoom*)new CEchoRoom((CNetRoomServer*)pParam, iIdx, iMaxPlayer, dwFrame);
}

