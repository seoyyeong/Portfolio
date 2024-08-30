#include "CAstar.h"

void CAstar::Initialize(void)
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
    CPathFinder::Initialize(iMapWidth, iMapHeight, 1, MANHATTAN, EUCLIDEAN);
    iHweight = 1;
}

void CAstar::OnLogic(Node* pNode)
{
	CreateNode(pNode, LL);
	CreateNode(pNode, LD);
	CreateNode(pNode, LU);
	CreateNode(pNode, RR);
	CreateNode(pNode, RU);
	CreateNode(pNode, RD);
	CreateNode(pNode, UU);
	CreateNode(pNode, DD);
	pNode->_type = Node::CLOSE;
}

void CAstar::OnPathFindComplete(void)
{
	Node* pParent = End._parent;

	while (pParent != &Start)
	{
		pParent->_type = Node::PATH;
		pParent = pParent->_parent;
	}

}

CAstar::Node* CAstar::CreateNode(Node* pParent, int Direction)
{
    int X;
    int Y;
    int Gweight;
    float newG;
    float newH;
    float newF;

    switch (Direction)
    {
    case LL:
        X = pParent->_X - 1;
        Y = pParent->_Y;
        Gweight = 10;
        break;
    case LD:
        X = pParent->_X - 1;
        Y = pParent->_Y + 1;
        Gweight = 14;
        break;
    case LU:
        X = pParent->_X - 1;
        Y = pParent->_Y - 1;
        Gweight = 14;
        break;
    case RR:
        X = pParent->_X + 1;
        Y = pParent->_Y;
        Gweight = 10;
        break;
    case RU:
        X = pParent->_X + 1;
        Y = pParent->_Y - 1;
        Gweight = 14;
        break;
    case RD:
        X = pParent->_X + 1;
        Y = pParent->_Y + 1;
        Gweight = 14;
        break;
    case UU:
        X = pParent->_X;
        Y = pParent->_Y - 1;
        Gweight = 10;
        break;
    case DD:
        X = pParent->_X;
        Y = pParent->_Y + 1;
        Gweight = 10;
        break;
    default:
        X = pParent->_X;
        Y = pParent->_Y;
        Gweight = 10;
    }


    if (X < 0 || X >= iMaxWidth || Y < 0 || Y >= iMaxHeight)
        return nullptr;

    if (Grid[Y][X] == &Block)
        return nullptr;

    if (Grid[Y][X] == &End)
    {
        End._parent = pParent;
        OpenList.push(&End);
        return &End;
    }

    ///////////가중치 계산
    if (iGform == EUCLIDEAN)
        newG = EuclideanDistance(Start._X, Start._Y, X, Y);
    else
        newG = pParent->_G + Gweight;

    if (iHform == EUCLIDEAN)
        newH = EuclideanDistance(X, Y, End._X, End._Y);
    else
        newH = ManhattanDistance(X, Y, End._X, End._Y);

    newF = newG + (newH * iHweight);


    if (Grid[Y][X] == nullptr)
    {
        Node* pNode = NodePool->Alloc();

        Grid[Y][X] = pNode;
        pNode->_X = X;
        pNode->_Y = Y;
        pNode->_type = Node::OPEN;
        pNode->_parent = pParent;
        pNode->_G = newG;
        pNode->_H = newH;
        pNode->_F = newF;

        //OpenList.push_back(pNode);
        OpenList.push(pNode);
    }
    else
    {
        if (Grid[Y][X]->_G > newG)
        {
            Grid[Y][X]->_parent = pParent;
            Grid[Y][X]->_F = newF;
            Grid[Y][X]->_G = newG;
            Grid[Y][X]->_H = newH;
            //Grid[Y][X]->_using = OPEN;
            //OpenList.push(Grid[Y][X]);
        }
    }

    return Grid[Y][X];
}

void CAstar::Render(HDC hdc)
{

   Node* pParent;
   Node* pNode;
   WCHAR word[10];


    int iX = 0;
    int iY = 0;
    bool TextPrint = false;

    HPEN hOldPen = (HPEN)SelectObject(hdc, hParentPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hTileBrush);
    SelectObject(hdc, GetStockObject(NULL_PEN));

    if (GRID_SIZE >= 25)
        TextPrint = true;

    ////길찾기 노드
    for (int iCntW = 0; iCntW < iMaxWidth; iCntW++)
    {
        for (int iCntH = 0; iCntH < iMaxHeight; iCntH++)
        {

            pNode = Grid[iCntH][iCntW];

            if (pNode != nullptr && pNode != &Block)
            {
                iX = iCntW * GRID_SIZE;
                iY = iCntH * GRID_SIZE;

                if (pNode->_type == CPathFinder::Node::OPEN)
                {
                    hOldBrush = (HBRUSH)SelectObject(hdc, hOpenBrush);
                    SelectObject(hdc, GetStockObject(NULL_PEN));
                    Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);

                    if (TextPrint)
                    {
                        wsprintfW(word, L"%d", (int)pNode->_F);
                        TextOutW(hdc, iX, iY, word, lstrlen(word));
                        wsprintfW(word, L"%d", (int)pNode->_H);
                        TextOutW(hdc, iX + 50, iY + 20, word, lstrlen(word));
                        wsprintfW(word, L"%d", (int)pNode->_G);
                        TextOutW(hdc, iX + 50, iY, word, lstrlen(word));
                    }
                }
                else if (pNode->_type == CPathFinder::Node::CLOSE)
                {
                    hOldBrush = (HBRUSH)SelectObject(hdc, hCloseBrush);
                    SelectObject(hdc, GetStockObject(NULL_PEN));
                    Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);

                    if (TextPrint)
                    {
                        wsprintfW(word, L"%d", (int)pNode->_H);
                        TextOutW(hdc, iX + 20, iY + 20, word, lstrlen(word));
                        wsprintfW(word, L"%d", (int)pNode->_G);
                        TextOutW(hdc, iX + 20, iY, word, lstrlen(word));
                        wsprintfW(word, L"%d", (int)pNode->_F);
                        TextOutW(hdc, iX, iY, word, lstrlen(word));
                    }
                }
                else if (pNode->_type == CPathFinder::Node::PATH)
                {
                    hOldBrush = (HBRUSH)SelectObject(hdc, hPathBrush);
                    SelectObject(hdc, GetStockObject(NULL_PEN));
                    Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);

                    if (TextPrint)
                    {
                        wsprintfW(word, L"%d", (int)pNode->_F);
                        TextOutW(hdc, iX, iY, word, lstrlen(word));
                        wsprintfW(word, L"%d", (int)pNode->_H);
                        TextOutW(hdc, iX + 50, iY + 20, word, lstrlen(word));
                        wsprintfW(word, L"%d", (int)pNode->_G);
                        TextOutW(hdc, iX + 50, iY, word, lstrlen(word));
                    }

                }

                ///////////부모노드 연결점
                pParent = pNode->_parent;

                if (pParent != nullptr)
                {
                    int nX;
                    int nY;
                    hOldPen = (HPEN)SelectObject(hdc, hParentPen);

                    if (pParent->_X - 1 == iCntW && pParent->_Y == iCntH)
                    {
                        nX = iX + GRID_SIZE;
                        nY = iY + GRID_SIZE / 2;
                    }
                    if (pParent->_X - 1 == iCntW && pParent->_Y + 1 == iCntH)
                    {
                        nX = iX + GRID_SIZE;
                        nY = iY;
                    }
                    if (pParent->_X - 1 == iCntW && pParent->_Y - 1 == iCntH)
                    {
                        nX = iX + GRID_SIZE;
                        nY = iY + GRID_SIZE;
                    }
                    if (pParent->_X + 1 == iCntW && pParent->_Y == iCntH) //RR
                    {
                        nX = iX;
                        nY = iY + GRID_SIZE / 2;
                    }
                    if (pParent->_X + 1 == iCntW && pParent->_Y - 1 == iCntH) //RU
                    {
                        nX = iX;
                        nY = iY + GRID_SIZE;
                    }
                    if (pParent->_X + 1 == iCntW && pParent->_Y + 1 == iCntH) //RD
                    {
                        nX = iX;
                        nY = iY;
                    }
                    if (pParent->_X == iCntW && pParent->_Y - 1 == iCntH) //UU
                    {
                        nX = iX + GRID_SIZE / 2;
                        nY = iY + GRID_SIZE;
                    }
                    if (pParent->_X == iCntW && pParent->_Y + 1 == iCntH) //UD
                    {
                        nX = iX + GRID_SIZE / 2;
                        nY = iY;
                    }

                    MoveToEx(hdc, iX + GRID_SIZE / 2, iY + GRID_SIZE / 2, NULL);
                    LineTo(hdc, nX, nY);

                    SelectObject(hdc, hOldPen);
                }


            }

            iX = iCntW * GRID_SIZE;
            iY = iCntH * GRID_SIZE;

            if (Grid[iCntH][iCntW] == &Block)
            {
                hOldBrush = (HBRUSH)SelectObject(hdc, hTileBrush);
                SelectObject(hdc, GetStockObject(NULL_PEN));
                Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
            }
            else if (Grid[iCntH][iCntW] == &Start)
            {
                hOldBrush = (HBRUSH)SelectObject(hdc, hStartBrush);
                SelectObject(hdc, GetStockObject(NULL_PEN));
                Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
            }
            else if (Grid[iCntH][iCntW] == &End)
            {
                hOldBrush = (HBRUSH)SelectObject(hdc, hEndBrush);
                SelectObject(hdc, GetStockObject(NULL_PEN));
                Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
            }

        }
    }

    SelectObject(hdc, hOldBrush);
    RenderGrid(hdc);
    RenderText(hdc);
}