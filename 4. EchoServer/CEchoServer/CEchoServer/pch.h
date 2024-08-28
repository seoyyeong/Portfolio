
#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment (lib, "winmm")
#pragma comment(lib, "Synchronization")
#pragma comment(lib, "Pdh")
#pragma comment (lib, "Dbghelp.lib")


#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>

#include <iostream>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <Dbghelp.h>
#include <crtdbg.h>
#include <process.h>
#include <conio.h>
#include <psapi.h>
#include <Pdh.h>
#include <mutex>

#include <wchar.h>
#include <thread>
#include <string>
#include <queue>
#include <list>
#include <vector>
#include <unordered_map>
#include <set>
#include <strsafe.h>
#include <math.h>
#include <functional>
#include <stdarg.h>

#define CRASH() CCrashDump::Crash()


#include "CTlsProfile.h"
#include "CRingBuffer.h"
#include "CCrashDump.h"
#include "CLog.h"
#include "CLockFreeList.h"
#include "CLockFreeStack.h"
#include "CLockFreeQueue.h"
#include "CObjectFreeListLock.h"
#include "CPacket.h"
#include "CTlsPool.h"
#include "CPacket.h"
#include "CHardwareStatus.h"
#include "CFrame.h"
#include "CParser.h"