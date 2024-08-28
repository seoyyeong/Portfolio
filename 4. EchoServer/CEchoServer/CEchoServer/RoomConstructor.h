#pragma once

//상속받는  Room Class 추가
#include "pch.h"
#include "CAuthRoom.h"
#include "CEchoRoom.h"



using RoomConstructor = std::function<CRoom* (void*, int, int, DWORD)>;

//-----------------------------------Room 생성 함수 포인터 배열
extern std::vector<RoomConstructor> RoomConstructors;


CRoom* CreateAuthRoom(void* pParam, int iIdx, int iMaxPlayer, DWORD dwFrame);

CRoom* CreateEchoRoom(void* pParam, int iIdx, int iMaxPlayer, DWORD dwFrame);


