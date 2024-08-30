#include "CJPS.h"


void CJPS::Initialize(void)
{
	CParser Parse;
	int iMapWidth;
	int iMapHeight;
	Parse.LoadFile(L"PathFind.txt");
	if (Parse.GetValue(L"PathFind", L"BLOCK_WIDTH", iMapWidth) == FALSE)
	{
		iMapWidth = GRID_WIDTH;
	}
	if (Parse.GetValue(L"PathFind", L"BLOCK_HEIGHT", iMapHeight) == FALSE)
	{
		iMapHeight = GRID_HEIGHT;
	}
	iCheckedIdx = 0;
	Checked = new int* [iMapHeight];
	for (int i = 0; i < iMapHeight; i++)
	{
		Checked[i] = new int[iMapWidth];
		for (int j = 0; j < iMapWidth; j++)
		{
			Checked[i][j] = -1;
		}
	}

	ColorList.resize(10000);

	for (int i = 0; i < 10000; i++)
	{
		ColorList[i].resize(3);
		for (int j = 0; j < 3; j++)
		{
			ColorList[i][j] = rand() % 255 + 180;
			if (ColorList[i][j] > 255)
			{
				ColorList[i][j] = 255;
			}
			if (ColorList[i][j] == 0)
			{
				ColorList[i][j] = 10;
			}
		}
	}
	CPathFinder::Initialize(iMapWidth, iMapHeight);
}

void CJPS::OnLogic(Node* pNode)
{
	Node* Node;
	int x = pNode->_X;
	int y = pNode->_Y;
	int g = pNode->_G;

	pNode->_type = Node::CLOSE;
	switch (pNode->_dir)
	{
	case LL:
		CheckPath(pNode, LL);

		if (y > 0)
		{
			if (Grid[y - 1][x] == &Block)
			{
				CheckPath(pNode, LU);
			}
		}

		if (y < iMaxHeight - 1)
		{
			if (Grid[y + 1][x] == &Block)
			{
				CheckPath(pNode, LD);
			}
		}

		break;

	case LU:
		CheckPath(pNode, LL);
		CheckPath(pNode, UU);
		CheckPath(pNode, LU);
		if (y < iMaxHeight - 1)
		{
			if (Grid[y + 1][x] == &Block)
			{
				CheckPath(pNode, LD);
			}
		}
		if (x < iMaxWidth - 1)
		{
			if (Grid[y][x + 1] == &Block)
			{
				CheckPath(pNode, RU);
			}
		}

		break;
		
	case LD:
		CheckPath(pNode, LL);
		CheckPath(pNode, DD);
		CheckPath(pNode, LD);

		if (y > 0)
		{
			if (Grid[y - 1][x] == &Block)
			{
				CheckPath(pNode, LU);
			}
		}
		if (x < iMaxWidth - 1)
		{
			if (Grid[y][x + 1] == &Block)
			{
				CheckPath(pNode, RD);
			}
		}

		break;

	case RR:
		CheckPath(pNode, RR);
		if (y > 0)
		{
			if (Grid[y - 1][x] == &Block)
			{
				CheckPath(pNode, RU);
			}
		}
		if (y < iMaxHeight - 1)
		{
			if (Grid[y + 1][x] == &Block)
			{
				CheckPath(pNode, RD);
			}
		}

		break;

	case RU:
		CheckPath(pNode, RR);
		CheckPath(pNode, UU);
		CheckPath(pNode, RU);

		if (y < iMaxHeight - 1)
		{
			if (Grid[y + 1][x] == &Block)
			{
				CheckPath(pNode, RD);
			}
		}
		if (x > 0)
		{
			if (Grid[y][x - 1] == &Block)
			{
				CheckPath(pNode, LU);
			}
		}

		break;

	case RD:
		CheckPath(pNode, RR);
		CheckPath(pNode, RD);
		CheckPath(pNode, DD);

		if (x > 0)
		{
			if (Grid[y][x - 1] == &Block)
			{
				CheckPath(pNode, LD);
			}
		}
		if (y > 0)
		{
			if (Grid[y - 1][x] == &Block)
			{
				CheckPath(pNode, RU);
			}
		}

		break;

	case UU:
		CheckPath(pNode, UU);
		if (x > 0)
		{
			if (Grid[y][x - 1] == &Block)
			{
				CheckPath(pNode, LU);
			}
		}
		if (x < iMaxWidth - 1)
		{
			if (Grid[y][x + 1] == &Block)
			{
				CheckPath(pNode, RU);
			}
		}

		break;

	case DD:
		CheckPath(pNode, DD);
		if (x > 0)
		{
			if (Grid[y][x - 1] == &Block)
			{
				CheckPath(pNode, LD);
			}
		}
		if (x < iMaxWidth - 1)
		{
			if (Grid[y][x + 1] == &Block)
			{
				CheckPath(pNode, RD);
			}
		}

		break;
	default:
		CheckPath(pNode, LL);
		CheckPath(pNode, RR);
		CheckPath(pNode, UU);
		CheckPath(pNode, DD);
		CheckPath(pNode, LU);
		CheckPath(pNode, LD);
		CheckPath(pNode, RU);
		CheckPath(pNode, RD);

	}

	iCheckedIdx++;
}

void CJPS::OnClear(void)
{
	iCheckedIdx = 0;
	for (int i = 0; i < iMaxHeight; i++)
	{
		for (int j = 0; j < iMaxWidth; j++)
		{
			Checked[i][j] = -1;
		}
	}

}

void CJPS::OnPathFindComplete(void)
{
	if (iFixPath)
	{
		FixPath();
	}
}

CJPS::Node* CJPS::CreateNode(Node* pParent, int X, int Y, int G, int Direction)
{
	Node* newNode = nullptr;

	float newH;
	float newF;

	newH = EuclideanDistance(X, Y,End._X, End._Y);
	newF = G + newH;

	if (Grid[Y][X] != nullptr)
	{

		if (Grid[Y][X] == &End)
		{
			End._parent = pParent;
			OpenList.push(&End);
			return nullptr;
		}

		if (Grid[Y][X]->_G > G)
		{
			Grid[Y][X]->_F = newF;
			Grid[Y][X]->_parent = pParent;
			Grid[Y][X]->_dir = Direction;
			return Grid[Y][X];
		}

	}
	else
	{
		newNode = NodePool->Alloc();

		Grid[Y][X] = newNode;
		newNode->_X = X;
		newNode->_Y = Y;
		newNode->_G = G;
		newNode->_H = newH;
		newNode->_F = G + newH;
		newNode->_dir = Direction;
		newNode->_parent = pParent;
		newNode->_fixedparent = nullptr;
		newNode->_type = Node::OPEN;

		OpenList.push(newNode);
	}

	return newNode;
}

bool CJPS::CheckPath(Node* pNode, int dir)
{
	coord Coord;
	bool ret;

	switch (dir)
	{
	case LL:
	case RR:
	case UU:
	case DD:
		ret = CheckStraight(pNode, pNode->_X, pNode->_Y, pNode->_G, dir, Coord);
		break;
	case LU:
	case LD:
	case RU:
	case RD:
		ret = CheckDiagonal(pNode, dir, Coord);
		break;
	default:
		return FALSE;
	}

	if (ret == TRUE)
	{
		CreateNode(pNode, Coord._X, Coord._Y, Coord._G, dir);
		return TRUE;
	}

	return FALSE;
}

bool CJPS::CheckStraight(Node* pNode, int nX, int nY, int nG, int dir, coord& Coord)
{
	int X = nX;
	int Y = nY;
	int G = nG;

	switch (dir)
	{
	case LL:
		while (1)
		{
			X--;
			G += 10;

			if (CheckBlockValid(X, Y) == FALSE)
			{
				return FALSE;
			}

			Coord._G = G;
			Coord._X = X;
			Coord._Y = Y;

			if (Grid[Y][X] == &End)
			{
				return TRUE;
			}

			Checked[Y][X] = iCheckedIdx;

			if (X > 0)
			{
				if (Y < iMaxHeight - 1)
				{
					if (Grid[Y + 1][X] == &Block && Grid[Y + 1][X - 1] != &Block)
					{
						Coord._G = G;
						Coord._X = X;
						Coord._Y = Y;
						return TRUE;
					}
				}
				if (Y > 0)
				{
					if (Grid[Y - 1][X] == &Block && Grid[Y - 1][X - 1] != &Block)
					{
						Coord._G = G;
						Coord._X = X;
						Coord._Y = Y;
						return TRUE;
					}
				}
			}
		}

		break;

	case RR:
		while (1)
		{
			X++;
			G += 10;

			if (CheckBlockValid(X, Y) == FALSE)
			{
				return FALSE;
			}

			Coord._G = G;
			Coord._X = X;
			Coord._Y = Y;

			if (Grid[Y][X] == &End)
			{
				return TRUE;
			}

			Checked[Y][X] = iCheckedIdx;

			if (X < iMaxWidth - 1)
			{
				if (Y < iMaxHeight - 1)
				{

					if (Grid[Y + 1][X] == &Block && Grid[Y + 1][X + 1] != &Block)
					{
						Coord._G = G;
						Coord._X = X;
						Coord._Y = Y;
						return TRUE;
					}
				}
				if (Y > 0)
				{
					if (Grid[Y - 1][X] == &Block && Grid[Y - 1][X + 1] != &Block)
					{
						Coord._G = G;
						Coord._X = X;
						Coord._Y = Y;
						return TRUE;
					}
				}
			}
		}

		break;

	case UU:
		while (1)
		{
			Y--;
			G += 10;

			if (CheckBlockValid(X, Y) == FALSE)
			{
				return FALSE;
			}

			Coord._G = G;
			Coord._X = X;
			Coord._Y = Y;

			if (Grid[Y][X] == &End)
			{
				return TRUE;
			}

			Checked[Y][X] = iCheckedIdx;

			if (Y > 0)
			{
				if (X < iMaxWidth - 1)
				{
					if (Grid[Y][X + 1] == &Block && Grid[Y - 1][X + 1] != &Block)
					{
						Coord._G = G;
						Coord._X = X;
						Coord._Y = Y;
						return TRUE;
					}
				}
				if (X > 0)
				{
					if (Grid[Y][X - 1] == &Block && Grid[Y - 1][X - 1] != &Block)
					{
						Coord._G = G;
						Coord._X = X;
						Coord._Y = Y;
						return TRUE;
					}
				}

			}
		}

		break;
	case DD:
		while (1)
		{
			Y++;
			G += 10;

			if (CheckBlockValid(X, Y) == FALSE)
			{
				return FALSE;
			}

			Coord._G = G;
			Coord._X = X;
			Coord._Y = Y;

			if (Grid[Y][X] == &End)
			{
				return TRUE;
			}

			Checked[Y][X] = iCheckedIdx;

			if (Y < iMaxHeight - 1)
			{
				if (X < iMaxWidth - 1)
				{

					if (Grid[Y][X + 1] == &Block && Grid[Y + 1][X + 1] != &Block)
					{
						Coord._G = G;
						Coord._X = X;
						Coord._Y = Y;
						return TRUE;
					}
				}
				if (X > 0)
				{
					if (Grid[Y][X - 1] == &Block && Grid[Y + 1][X - 1] != &Block)
					{
						Coord._G = G;
						Coord._X = X;
						Coord._Y = Y;
						return TRUE;
					}
				}

			}
		}

		break;
	}

	return FALSE;
}

bool CJPS::CheckDiagonal(Node* pNode, int dir, coord& Coord)
{
	int X = pNode->_X;
	int Y = pNode->_Y;
	int G = pNode->_G;

	switch (dir)
	{
	case LU:
		while (1)
		{
			X--;
			Y--;
			G += 14;

			if (CheckBlockValid(X, Y) == FALSE)
			{
				return FALSE;
			}

			Coord._G = G;
			Coord._X = X;
			Coord._Y = Y;

			if (Grid[Y][X] == &End)
			{
				return TRUE;
			}

			Checked[Y][X] = iCheckedIdx;


			if (X > 0 && Y < iMaxHeight - 1)
			{
				if (Grid[Y + 1][X] == &Block && Grid[Y + 1][X - 1] != &Block)
				{
					return TRUE;
				}

			}
			if (Y > 0 && X < iMaxWidth - 1)
			{
				if (Grid[Y][X + 1] == &Block && Grid[Y - 1][X + 1] != &Block)
				{
					return TRUE;
				}
			}

			if (CheckStraight(pNode, X, Y, G, LL, Coord))
			{
				CreateNode(pNode, X, Y, G, dir);
				//CreateNode(pNode, X, Y, G, LL);
				return FALSE;
			}
			if (CheckStraight(pNode, X, Y, G, UU, Coord))
			{
				CreateNode(pNode, X, Y, G, dir);
				//CreateNode(pNode, X, Y, G, UU);
				return FALSE;
			}
		}

		break;
	case LD:
		while (1)
		{
			X--;
			Y++;
			G += 14;

			if (CheckBlockValid(X, Y) == FALSE)
			{
				return FALSE;
			}

			Coord._G = G;
			Coord._X = X;
			Coord._Y = Y;

			if (Grid[Y][X] == &End)
			{
				return TRUE;
			}

			Checked[Y][X] = iCheckedIdx;

			if (X > 0)
			{
				if (Grid[Y - 1][X] == &Block && Grid[Y - 1][X - 1] != &Block)
				{
					return TRUE;
				}

			}
			if (Y < iMaxHeight - 1)
			{
				if (Grid[Y][X + 1] == &Block && Grid[Y + 1][X + 1] != &Block)
				{
					return TRUE;
				}
			}


			if (CheckStraight(pNode, X, Y, G, LL, Coord))
			{
				CreateNode(pNode, X, Y, G, dir);
				return FALSE;
			}
			if (CheckStraight(pNode, X, Y, G, DD, Coord))
			{
				CreateNode(pNode, X, Y, G, dir);
				return FALSE;
			}
		}
		break;

	case RU:
		while (1)
		{
			X++;
			Y--;
			G += 14;

			if (CheckBlockValid(X, Y) == FALSE)
			{
				return FALSE;
			}

			Coord._G = G;
			Coord._X = X;
			Coord._Y = Y;

			if (Grid[Y][X] == &End)
			{
				return TRUE;
			}

			Checked[Y][X] = iCheckedIdx;


			if (X < iMaxWidth - 1)
			{
				if (Grid[Y + 1][X] == &Block && Grid[Y + 1][X + 1] != &Block)
				{
					return TRUE;
				}

			}
			if (Y > 0)
			{
				if (Grid[Y][X - 1] == &Block && Grid[Y - 1][X - 1] != &Block)
				{
					return TRUE;
				}
			}
			if (CheckStraight(pNode, X, Y, G, RR, Coord))
			{
				CreateNode(pNode, X, Y, G, dir);
				return FALSE;
			}
			if (CheckStraight(pNode, X, Y, G, UU, Coord))
			{
				CreateNode(pNode, X, Y, G, dir);
				return FALSE;
			}
		}
		break;

	case RD:
		while (1)
		{
			X++;
			Y++;
			G += 14;

			if (CheckBlockValid(X, Y) == FALSE)
			{
				return FALSE;
			}

			Coord._G = G;
			Coord._X = X;
			Coord._Y = Y;

			if (Grid[Y][X] == &End)
			{
				return TRUE;
			}

			Checked[Y][X] = iCheckedIdx;

			if (X < iMaxWidth)
			{
				if (Grid[Y - 1][X] == &Block && Grid[Y - 1][X + 1] != &Block)
				{
					return TRUE;
				}

			}
			if (Y < iMaxHeight - 1)
			{
				if (Grid[Y][X - 1] == &Block && Grid[Y + 1][X - 1] != &Block)
				{
					return TRUE;
				}
			}
			if (CheckStraight(pNode, X, Y, G, RR, Coord))
			{
				CreateNode(pNode, X, Y, G, dir);
				return FALSE;
			}
			if (CheckStraight(pNode, X, Y, G, DD, Coord))
			{
				CreateNode(pNode, X, Y, G, dir);
				return FALSE;
			}
		}
		break;
	}
	return FALSE;
}

void CJPS::Render(HDC hdc)
{

	Node* pNode;
	Node* pParent;

	int iX = 0;
	int iY = 0;
	int pX = 0;
	int pY = 0;
	int cnt = 0;
	bool TextPrint = false;
	WCHAR word[10];

	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hTileBrush);


	if (GRID_SIZE >= 25)
		TextPrint = true;

	////길찾기 노드


	for (int iCntW = 0; iCntW < iMaxWidth; iCntW++)
	{
		for (int iCntH = 0; iCntH < iMaxHeight; iCntH++)
		{

			iX = iCntW * GRID_SIZE;
			iY = iCntH * GRID_SIZE;

			//checked grid
			if (Checked[iCntH][iCntW] > -1)
			{
				SelectObject(hdc, GetStockObject(DC_BRUSH));
				SetDCBrushColor(hdc, RGB(ColorList[Checked[iCntH][iCntW]][0], ColorList[Checked[iCntH][iCntW]][1], ColorList[Checked[iCntH][iCntW]][2]));
				Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);

			}

			//blocks
			if (Grid[iCntH][iCntW] == &Block)
			{
				hOldBrush = (HBRUSH)SelectObject(hdc, hTileBrush);
				Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
			}
			else if (Grid[iCntH][iCntW] == &Start)
			{
				hOldBrush = (HBRUSH)SelectObject(hdc, hStartBrush);
				Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
			}
			else if (Grid[iCntH][iCntW] == &End)
			{
				hOldBrush = (HBRUSH)SelectObject(hdc, hEndBrush);
				Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
			}

			//node

			pNode = Grid[iCntH][iCntW];

			if (pNode != nullptr && pNode != &Block && pNode != &Start && pNode != &End)
			{
				iX = iCntW * GRID_SIZE;
				iY = iCntH * GRID_SIZE;
				if (pNode->_type == Node::OPEN)
				{
					hOldBrush = (HBRUSH)SelectObject(hdc, hCloseBrush);
				}
				else
				{
					hOldBrush = (HBRUSH)SelectObject(hdc, hOpenBrush);
				}
				Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);

			}
		}
	}
	SelectObject(hdc, hOldBrush);

	RenderGrid(hdc);
	//////////////////////////////////////////////////////////////////////////////////path line

	HPEN hOldPen = (HPEN)SelectObject(hdc, hPathPen);

	SelectObject(hdc, GetStockObject(NULL_PEN));

	SelectObject(hdc, hPathPen);
	///////////path 그리기
	if (iResult == TRUE)
	{
	pNode = &End;
	while (pNode != &Start && pNode != nullptr)
	{
		pParent = pNode->_parent;
		if (pParent == nullptr)
		{
			break;
		}
		iX = pNode->_X * GRID_SIZE;
		iY = pNode->_Y * GRID_SIZE;
		pX = pParent->_X * GRID_SIZE;
		pY = pParent->_Y * GRID_SIZE;

		MoveToEx(hdc, pX + GRID_SIZE / 2, pY + GRID_SIZE / 2, NULL);
		LineTo(hdc, iX + GRID_SIZE / 2, iY + GRID_SIZE / 2);
		if (TextPrint)
		{
			wsprintfW(word, L"%d %d", ++cnt, (int)pNode->_F);
			TextOutW(hdc, iX, iY, word, lstrlen(word));
		}

		pNode = pParent;
	}

	pNode = &End;
	SelectObject(hdc, hFixedPathPen);


		while (pNode != &Start)
		{
			pParent = pNode->_fixedparent;
			if (pParent == nullptr)
			{
				pParent = pNode->_parent;
				if (pParent == nullptr)
				{
					break;
				}
			}
			iX = pNode->_X * GRID_SIZE;
			iY = pNode->_Y * GRID_SIZE;
			pX = pParent->_X * GRID_SIZE;
			pY = pParent->_Y * GRID_SIZE;

			MoveToEx(hdc, pX + GRID_SIZE / 2, pY + GRID_SIZE / 2, NULL);
			LineTo(hdc, iX + GRID_SIZE / 2, iY + GRID_SIZE / 2);

			pNode = pParent;
		}
	}


	RenderText(hdc);
	SelectObject(hdc, hOldPen);
}