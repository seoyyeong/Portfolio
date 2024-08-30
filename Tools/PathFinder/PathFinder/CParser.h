#pragma once

#include <iostream>
#include <Windows.h>
#include <unordered_map>
#include "CLog.h"


#define dfPARSE_LOOKUP_INDEX  5
#define dfPARSE_MAX_BUFSIZE   256

extern WCHAR g_EscapeLookup[dfPARSE_LOOKUP_INDEX];

class CParser
{


public:
	CParser();
	~CParser();

	bool LoadFile(const WCHAR* FileName);
	void ClearFile(void);

	bool GetValue(const WCHAR* BlockName, const WCHAR* TypeName, bool& bValue);
	bool GetValue(const WCHAR* BlockName, const WCHAR* TypeName, char* chValue, int iBufLen);
	bool GetValue(const WCHAR* BlockName, const WCHAR* TypeName, short& shValue);
	bool GetValue(const WCHAR* BlockName, const WCHAR* TypeName, int& iValue);
	bool GetValue(const WCHAR* BlockName, const WCHAR* TypeName, DWORD& dwValue);
	bool GetValue(const WCHAR* BlockName, const WCHAR* TypeName, float& fValue);
	bool GetValue(const WCHAR* BlockName, const WCHAR* TypeName, double& dValue);
	bool GetValue(const WCHAR* BlockName, const WCHAR* TypeName, __int64& iValue);
	bool GetValue(const WCHAR* BlockName, const WCHAR* TypeName, WCHAR* wchValue, int iBufLen);

private:

	struct stGroup
	{
		std::wstring GroupName;
		std::unordered_map<std::wstring, std::wstring> ValueMap;
	};

	void         SetGroup(WCHAR* pGroup);
	stGroup*     FindGroup(const WCHAR* GroupName);
	const WCHAR* FindValue(stGroup* pGroup, const WCHAR* TypeName);

	inline bool CheckEscapeWord(WCHAR ch)
	{
		for (int i = 0; i < dfPARSE_LOOKUP_INDEX; i++)
		{
			if (g_EscapeLookup[i] == ch)
			{
				return TRUE;
			}
		}

		return FALSE;
	}

	inline bool CheckFootNote(WCHAR* ch)
	{
		if (*ch == L'/' && *(ch + 1) == L'/')
		{
			return TRUE;
		}
		return FALSE;
	}

	inline bool CheckWord(WCHAR ch, WCHAR CheckWord)
	{
		if (ch == CheckWord)
		{
			return TRUE;
		}

		return FALSE;
	}

	inline bool CompareValue(WCHAR* pValue, const WCHAR* pCmp, int iLen)
	{
		for (int i = 0; i < iLen * 2; i++)
		{
			if (*(pValue + i) != *(pCmp + i))
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	FILE* pFile;
	std::vector<stGroup*> GroupList;

};