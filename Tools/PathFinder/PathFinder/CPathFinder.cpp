#include "CPathFinder.h"

CPathFinder::CPathFinder(void)
{
	iHweight = 1;
	iResult = FALSE;
	iType = -1;
	iFixPath = TRUE;

	iScreenX = 0;
	iScreenY = 0;

	Start._type = Node::START;
	End._type = Node::END;
	Block._type = Node::BLOCK;


	hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
	hParentPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));

	hTileBrush = CreateSolidBrush(RGB(100, 100, 100));
	hFixedTileBrush = CreateSolidBrush(RGB(254, 46, 247));
	hCheckedTileBrush = CreateSolidBrush(RGB(0, 0, 0));

	hStartBrush = CreateSolidBrush(RGB(255, 0, 0));
	hEndBrush = CreateSolidBrush(RGB(0, 255, 0));

	hOpenBrush = CreateSolidBrush(RGB(100, 100, 255));
	hCloseBrush = CreateSolidBrush(RGB(255, 255, 100));

	hPathBrush = CreateSolidBrush(RGB(255, 51, 153));
	hPathPen = CreatePen(PS_SOLID, 4, RGB(254, 154, 46));
	hFixedPathPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));

}

CPathFinder::~CPathFinder(void)
{
	for (int i = 0; i < iMaxHeight; i++)
	{
		delete[] Grid[i];
	}
	delete[] Grid;

	delete NodePool;

	DeleteObject(hGridPen);
	DeleteObject(hParentPen);
	DeleteObject(hTileBrush);
	DeleteObject(hFixedTileBrush);
	DeleteObject(hCheckedTileBrush);

	DeleteObject(hPathPen);
	DeleteObject(hFixedPathPen);
	DeleteObject(hPathBrush);
	DeleteObject(hStartBrush);
	DeleteObject(hEndBrush);
	DeleteObject(hOpenBrush);
	DeleteObject(hCloseBrush);
}

void CPathFinder::Initialize(int iMapWidth, int iMapHeight, int Hweight, int Gform, int Hform)
{

	NodePool = new CObjectFreeList<Node>(iMapWidth * iMapHeight, TRUE);
	iMaxWidth = iMapWidth;
	iMaxHeight = iMapHeight;
	iGform = Gform;
	iHform = Hform;
	iHweight = Hweight;

	Grid = new Node * *[iMapHeight];
	for (int i = 0; i < iMapHeight; i++)
	{
		Grid[i] = new Node * [iMapWidth];
		for (int j = 0; j < iMapWidth; j++)
		{
			Grid[i][j] = nullptr;
		}
	}
}

void CPathFinder::Clear(void)
{
	iResult = FALSE;
	End._parent = nullptr;
	End._fixedparent = nullptr;

	for (int i = 0; i < iMaxHeight; i++)
	{
		for (int j = 0; j < iMaxWidth; j++)
		{
			if (Grid[i][j] != &Block && Grid[i][j] != nullptr)
			{
				NodePool->Free(Grid[i][j]);
			}
			Grid[i][j] = nullptr;
		}
	}

	while (!OpenList.empty())
	{
		OpenList.pop();
	}
	OnClear();
}

void CPathFinder::ClearBlock(void)
{
	iResult = FALSE;
	End._parent = nullptr;
	End._fixedparent = nullptr;

	for (int i = 0; i < iMaxHeight; i++)
	{
		for (int j = 0; j < iMaxWidth; j++)
		{
			if (Grid[i][j] != &Start && Grid[i][j] != &End && Grid[i][j]!= nullptr)
			{
				if (Grid[i][j] != &Block)
				{
					NodePool->Free(Grid[i][j]);
				}
				Grid[i][j] = nullptr;
			}
		}
	}
	while (!OpenList.empty())
	{
		OpenList.pop();
	}
	OnClear();
}

void CPathFinder::ClearPath(void)
{
	iResult = FALSE;
	End._parent = nullptr;
	End._fixedparent = nullptr;

	for (int i = 0; i < iMaxHeight; i++)
	{
		for (int j = 0; j < iMaxWidth; j++)
		{
			if (Grid[i][j] != &Block && Grid[i][j] != &Start && Grid[i][j] != &End && Grid[i][j] != nullptr)
			{
				NodePool->Free(Grid[i][j]);
				Grid[i][j] = nullptr;
			}
		}
	}
	while (!OpenList.empty())
	{
		OpenList.pop();
	}
	OnClear();
}

void CPathFinder::RenderGrid(HDC hdc)
{
	int iX = 0;
	int iY = 0;

	SelectObject(hdc, GetStockObject(NULL_PEN));
	HPEN hOldPen = (HPEN)SelectObject(hdc, hGridPen);

	for (int iCntW = 0; iCntW <= iMaxWidth; iCntW++)
	{
		MoveToEx(hdc, iX, 0, NULL);
		LineTo(hdc, iX, iMaxHeight * GRID_SIZE);
		iX += GRID_SIZE;
	}

	for (int iCntH = 0; iCntH <= iMaxHeight; iCntH++)
	{
		MoveToEx(hdc, 0, iY, NULL);
		LineTo(hdc, iMaxWidth * GRID_SIZE, iY);
		iY += GRID_SIZE;
	}

	SelectObject(hdc, hOldPen);
}

void CPathFinder::RenderText(HDC hdc)
{
	WCHAR Text[256] = { '\0', };
	int len = 256;

	int iX = 1630;
	int iY = 70;

	swprintf_s(Text, len, L"****길찾기 프로그램****");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));

	iY += 50;
	iY += 20;
	swprintf_s(Text, 256, L"Map Size : %d * %d", iMaxWidth, iMaxHeight);
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 20;

	swprintf_s(Text, 256, L"[모드 설정]");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 20;
	swprintf_s(Text, 256, L"1. Astar / 2. JPS");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 20;
	if (iType == ASTAR)
	{
		swprintf_s(Text, 256, L"(현재 모드 : %s)", L"Astar");
	}
	else
	{
		swprintf_s(Text, 256, L"(현재 모드 : %s)", L"JPS");
	}
	TextOutW(hdc, iX, iY, Text, wcslen(Text));

	iY +=40;
	swprintf_s(Text, 256, L"[사용법]");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 20;
	swprintf_s(Text, len, L"좌클릭 + 드래그 : 장애물 그리기");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 20;

	swprintf_s(Text, len, L"스크롤 클릭 : 시작지점 지정");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 20;

	swprintf_s(Text, len, L"우클릭 : 종료지점 지정");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 20;

	swprintf_s(Text, len, L"스크롤 : 화면 크기 조정");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 20;

	//swprintf_s(Text, len, L"상하좌우 키: 화면 이동");
	//TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 40;

	swprintf_s(Text, len, L"엔터 : 길찾기(과정출력X)");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 20;
	swprintf_s(Text, len, L"스페이스바 : 길찾기(과정출력O)");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 20;
	swprintf_s(Text, len, L"백스페이스 : 전체 초기화");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 20;

	swprintf_s(Text, len, L"Z : 장애물 초기화");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));

	iY += 20;
	swprintf_s(Text, len, L"X : 경로만 초기화");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));

	iY += 40;
	swprintf_s(Text, 256, L"***Astar 알고리즘 설정***");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 23;
	swprintf_s(Text, 256, L"G : GForm 변경");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 23;
	if (iGform == EUCLIDEAN)
	{
		swprintf_s(Text, 256, L"(현재 모드 : %s)", L"Euclidean");
	}
	else
	{
		swprintf_s(Text, 256, L"(현재 모드 : %s)", L"Manhattan");
	}
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 23;
	swprintf_s(Text, 256, L"H : HForm 변경");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 23;
	if (iHform == EUCLIDEAN)
	{
		swprintf_s(Text, 256, L"(현재 모드 : %s)", L"Euclidean");
	}
	else
	{
		swprintf_s(Text, 256, L"(현재 모드 : %s)", L"Manhattan");
	}
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 70;
	swprintf_s(Text, 256, L"P : 현재 맵 대상 프로파일링");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 23;
	swprintf_s(Text, 256, L"Home : 랜덤 장애물 생성");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
	iY += 23;
	swprintf_s(Text, 256, L"(장애물 개수 파싱 파일 필요)");
	TextOutW(hdc, iX, iY, Text, wcslen(Text));
}


float CPathFinder::EuclideanDistance(int sX, int sY, int eX, int eY)
{
	int X = abs(sX - eX) * 10;
	int Y = abs(sY - eY) * 10;

	return sqrt(X * X + Y * Y);
}

int CPathFinder::ManhattanDistance(int sX, int sY, int eX, int eY)
{
	return abs(sX - eX) * 10 + abs(sY - eY) * 10;
}

bool CPathFinder::CheckBlockValid(int X, int Y)
{
	if (X < 0 || Y < 0 || X >= iMaxWidth || Y >= iMaxHeight)
	{
		return FALSE;
	}

	if (Grid[Y][X] == &Block)
	{
		return FALSE;
	}

	return TRUE;
}

void CPathFinder::FixPath(void)
{
	Node* pStart;
	Node* pEnd;
	Node* pSearch;
	Node* pParent;

	pStart = &Start;
	pEnd = &End;
	pParent = pEnd->_parent->_parent;

	while (1)
	{
		pSearch = pEnd;
		pParent = pSearch->_parent;

		while (pParent != nullptr)
		{
			if (!CheckPathBlocked(pSearch, pParent))
			{
				pSearch->_fixedparent = pParent;
			}
			pParent = pParent->_parent;
		}

		if (pEnd->_fixedparent != nullptr)
		{
			pEnd = pEnd->_fixedparent;
		}
		else
		{
			pEnd = pEnd->_parent;
		}
		if (pEnd == pStart)
		{
			break;
		}
	}

}

bool CPathFinder::CheckPathBlocked(Node* pStart, Node* pEnd)
{
	int sX;
	int sY;
	int eX;
	int eY;
	int mX;
	int mY;

	int dX;
	int dY;
	int p;

	sX = pStart->_X;
	sY = pStart->_Y;
	eX = pEnd->_X;
	eY = pEnd->_Y;
	mX = 1;
	mY = 1;

	dX = eX - sX;
	dY = eY - sY;

	if (dX < 0)
	{
		dX = -dX;
		mX = -1;
	}

	if (dY < 0)
	{
		dY = -dY;
		mY = -1;
	}

	if (dX > dY)
	{
		p = dY * 2 - dX;
		while (sX != eX)
		{
			if (Grid[sY][sX] == &Block)
			{
				return true;
			}

			if (p >= 0)
			{
				sY += mY;
				p -= dX * 2;
			}
			sX += mX;
			p += dY * 2;
		}
	}
	else
	{
		p = dX * 2 - dY;
		while (sY != eY)
		{
			if (Grid[sY][sX] == &Block)
			{
				return true;
			}

			if (p >= 0)
			{
				sX += mX;
				p -= dY * 2;
			}
			sY += mY;
			p += dX * 2;
	
		}
	}

	return false;

}


void CPathFinder::PathFind(int sX, int sY, int eX, int eY)
{

	Start._X = sX;
	Start._Y = sY;
	End._X = eX;
	End._Y = eY;
	Grid[sY][sX] = &Start;

	OpenList.push(&Start);
	End._X = eX;
	End._Y = eY;
	Grid[eY][eX] = &End;
	Node* pNode;

	if (CheckBlockValid(sX, sY) && CheckBlockValid(eX, eY))
	{
		while (1)
		{
			pNode = OpenList.top();
			OpenList.pop();

			if (pNode == &End)
			{
				iResult = TRUE;
				OnPathFindComplete();
				break;
			}

			OnLogic(pNode);

			if (OpenList.empty())
			{
				OnPathFindFailed();
				break;
			}

		}
	}

}


bool CPathFinder::PathFindByStep(int sX, int sY, int eX, int eY)
{

	Start._X = sX;
	Start._Y = sY;
	End._X = eX;
	End._Y = eY;
	Grid[sY][sX] = &Start;
	End._X = eX;
	End._Y = eY;
	Grid[eY][eX] = &End;

	OpenList.push(&Start);


	return TRUE;
}

bool CPathFinder::PathFindByStepUpdate(void)
{

	Node* pNode;

	if (OpenList.empty())
	{
		OnPathFindFailed();
		return FALSE;
	}

	pNode = OpenList.top();
	OpenList.pop();

	if (pNode == &End)
	{
		iResult = TRUE;
		OnPathFindComplete();
		return FALSE;
	}

	OnLogic(pNode);
	Sleep(5);
	return TRUE;
}
