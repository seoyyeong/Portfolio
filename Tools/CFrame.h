#pragma once

#include "pch.h"



class CFrame
{
public:

	CFrame()
	{
		timeBeginPeriod(1);

		dwTick = timeGetTime();
		dwFrame = 0;
		dwDelta = 0;
	}

	CFrame(DWORD iTime)
	{
		timeBeginPeriod(1);

		dwTick = timeGetTime();
		dwFrame = iTime;
		dwDelta = 0;
	}

	void SetFrame(DWORD dwSleepTime)
	{
		dwFrame = dwSleepTime;
	}

	void FrameControl(void)
	{
		dwDelta = timeGetTime() - dwTick;
		if (dwDelta < dwFrame)
		{
			Sleep(dwFrame - dwDelta);
		}

		dwTick += dwFrame;
	}

private:
	DWORD  dwDelta;
	DWORD  dwTick;
	DWORD  dwFrame;
};