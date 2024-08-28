#pragma once

//��ӹ޴�  Room Class �߰�
#include "pch.h"
#include "CAuthRoom.h"
#include "CEchoRoom.h"



using RoomConstructor = std::function<CRoom* (void*, int, int, DWORD)>;

//-----------------------------------Room ���� �Լ� ������ �迭
extern std::vector<RoomConstructor> RoomConstructors;


CRoom* CreateAuthRoom(void* pParam, int iIdx, int iMaxPlayer, DWORD dwFrame);

CRoom* CreateEchoRoom(void* pParam, int iIdx, int iMaxPlayer, DWORD dwFrame);


