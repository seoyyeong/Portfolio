
#include "CParser.h"

WCHAR g_EscapeLookup[dfPARSE_LOOKUP_INDEX] = { L'"', L'\t', L' ', 0x08, 0x09};



CParser::CParser()
{
}

CParser::~CParser()
{
	for (int i = 0; i < GroupList.size(); i++)
	{
		delete GroupList[i];
	}
}

bool CParser::LoadFile(const WCHAR* FileName)
{
	WCHAR   Buffer[dfPARSE_MAX_BUFSIZE];
	WCHAR*  p;
	errno_t err;
		
	err = _wfopen_s(&pFile, FileName, L"r,ccs=unicode");

	if (err != 0)
	{
		_LOG(L"PARSE", CLog::LEVEL_ERROR, L"Wrong File Input : fopen Error %s\n", FileName);
		return FALSE;
	}

	while (!feof(pFile))
	{
		fgetws(Buffer, dfPARSE_MAX_BUFSIZE, pFile);

		p = Buffer;
		while (*p)
		{
			if (*p == L':')
			{
				SetGroup(p + 1);
				break;
			}
			else if (CheckEscapeWord(*p))
			{
				p++;
				continue;
			}
			else
				break;
		}

	}
	fclose(pFile);

	return TRUE;
}

void CParser::ClearFile(void)
{
	for (int i = 0; i < GroupList.size(); i++)
	{
		delete GroupList[i];
	}
	GroupList.clear();
}

void CParser::SetGroup(WCHAR* pName)
{
	stGroup* pGroup;
	WCHAR   Buffer[dfPARSE_MAX_BUFSIZE];
	WCHAR* p;
	WCHAR* pKey;
	WCHAR* pValue;
	std::wstring str;

	pGroup = new stGroup;
	p = pName;

	while (*p!='\n')
	{
		p++;
	}

	*p = '\0';

	pGroup->GroupName = pName;

	//그룹 내 설정 텍스트 파싱 시작
	fgetws(Buffer, dfPARSE_MAX_BUFSIZE, pFile);
	p = Buffer;

	if (*p != L'{')
	{
		_LOG(L"PARSE", CLog::LEVEL_ERROR, L"Wrong File Input : Group Empty % s)", pName);
		return;
	}

	while (1)
	{
		memset(Buffer, 0, dfPARSE_MAX_BUFSIZE);
		fgetws(Buffer, dfPARSE_MAX_BUFSIZE, pFile);
		p = Buffer;

		if (*p == L'}')
		{
			break;
		}

		if (*p == '\n')
		{
			continue;
		}

		//각주, 공백문자 스킵하여 키 찾기
		while (CheckEscapeWord(*p))
		{
			p++;
		}

		if (*p == '\n')
		{
			continue;
		}



		if (CheckFootNote(p))
		{
			continue;
		}


		pKey = p;

		//키 이후 이어지는 value값 찾기
		while (!CheckEscapeWord(*p))
		{
			p++;
		}
		*p = L'\0';

		p++;
		while (CheckEscapeWord(*p))
		{
			p++;
		}

		if (*p == '=')
		{
			p++;
			while (CheckEscapeWord(*p))
			{
				p++;
			}
		}
		else
		{
			//key 다음 =문자가 보이지 않을 경우 스킵
			_LOG(L"PARSE", CLog::LEVEL_ERROR, L"Wrong File Input : Key Exist but no value %s)", pKey);
			continue;
		}

		pValue = p;
		while (!CheckEscapeWord(*p) && *p != '\n')
		{
			p++;
		}

		*p = L'\0';

		pGroup->ValueMap.insert(std::make_pair(pKey, pValue));

	}

	GroupList.push_back(pGroup);
}

bool CParser::GetValue(const WCHAR* GroupName, const WCHAR* TypeName, bool& bValue)
{
	const WCHAR* pValue;

	pValue = FindValue(FindGroup(GroupName), TypeName);
	if (pValue == nullptr)
	{
		return FALSE;
	}

	switch (*pValue)
	{
	case L'0':
	case L'f':
	case L'F':
		bValue = FALSE;
		break;
	case L'1':
	case L't':
	case L'T':
		bValue = TRUE;
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

//문자열과 구분되지 않으므로 바이너리 데이터 추출 시에만 활용
bool CParser::GetValue(const WCHAR* GroupName, const WCHAR* TypeName, char* chValue, int iBufLen)
{
	const WCHAR* pValue;
	char		 buf[50];
	errno_t      err;
	size_t       iLen;

	pValue = FindValue(FindGroup(GroupName), TypeName);
	if (pValue == nullptr)
	{
		return FALSE;
	}

	if (iBufLen == 1)
	{
		*chValue = (char)_wtoi(pValue);
	}
	else
	{
		err = wcstombs_s(&iLen, buf, 50, pValue, iBufLen + 1);
		if (err != 0)
		{
			return FALSE;
		}
		memcpy(chValue, buf, iBufLen);
	}
	return TRUE;
}

bool CParser::GetValue(const WCHAR* GroupName, const WCHAR* TypeName, short& shValue)
{
	const WCHAR* pValue;

	pValue = FindValue(FindGroup(GroupName), TypeName);
	if (pValue == nullptr)
	{
		return FALSE;
	}

	shValue = (short)_wtoi(pValue);
	return TRUE;
}

bool CParser::GetValue(const WCHAR* GroupName, const WCHAR* TypeName, int& iValue)
{
	const WCHAR* pValue;

	pValue = FindValue(FindGroup(GroupName), TypeName);
	if (pValue == nullptr)
	{
		return FALSE;
	}

	iValue = _wtoi(pValue);
	return TRUE;
}

bool CParser::GetValue(const WCHAR* GroupName, const WCHAR* TypeName, DWORD& dwValue)
{
	const WCHAR* pValue;

	pValue = FindValue(FindGroup(GroupName), TypeName);
	if (pValue == nullptr)
	{
		return FALSE;
	}

	dwValue = (DWORD)_wtoi(pValue);
	return TRUE;
}

bool CParser::GetValue(const WCHAR* GroupName, const WCHAR* TypeName, float& fValue)
{
	const WCHAR* pValue;

	pValue = FindValue(FindGroup(GroupName), TypeName);
	if (pValue == nullptr)
	{
		return FALSE;
	}

	fValue = _wtof(pValue);
	return TRUE;
}

bool CParser::GetValue(const WCHAR* GroupName, const WCHAR* TypeName, double& dValue)
{
	const WCHAR* pValue;

	pValue = FindValue(FindGroup(GroupName), TypeName);
	if (pValue == nullptr)
	{
		return FALSE;
	}

	dValue = _wtof(pValue);
	return TRUE;
}

bool CParser::GetValue(const WCHAR* GroupName, const WCHAR* TypeName, __int64& iValue)
{
	const WCHAR* pValue;

	pValue = FindValue(FindGroup(GroupName), TypeName);
	if (pValue == nullptr)
	{
		return FALSE;
	}

	iValue = _wtoi64(pValue);
	return TRUE;
}

bool CParser::GetValue(const WCHAR* GroupName, const WCHAR* TypeName, WCHAR* wchValue, int iBufLen)
{
	const WCHAR* pValue;

	pValue = FindValue(FindGroup(GroupName), TypeName);
	if (pValue == nullptr)
	{
		return FALSE;
	}

	wcscpy_s(wchValue, iBufLen, pValue);

	return TRUE;
}

CParser::stGroup* CParser::FindGroup(const WCHAR* GroupName)
{
	for (int i = 0; i < GroupList.size(); i++)
	{
		if (GroupList[i]->GroupName == GroupName)
		{
			return GroupList[i];
		}
	}

	return nullptr;
}

const WCHAR* CParser::FindValue(stGroup* pGroup, const WCHAR* TypeName)
{
	std::unordered_map<std::wstring, std::wstring>::iterator iter;

	if (pGroup == nullptr)
	{
		return nullptr;
	}

	iter = pGroup->ValueMap.find(TypeName);
	if (iter == pGroup->ValueMap.end())
	{
		return nullptr;
	}

	return iter->second.c_str();
}