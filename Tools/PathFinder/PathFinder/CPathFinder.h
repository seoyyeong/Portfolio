#pragma once

#define GRID_SIZE_DEFAULT 20
#define GRID_WIDTH 400
#define GRID_HEIGHT 200

#include <Windows.h>
#include <cmath>
#include <queue>
#include <vector>
#include "CObjectFreeListLock.h"
#include "CParser.h"


class CPathFinder
{

public:
	enum eDistanceForm
	{
		EUCLIDEAN = 0,
		MANHATTAN
	};
	enum eDirection
	{
		LL = 0, LD, LU, RR, RU, RD, UU, DD
	};
	enum eRenderLevel
	{
		LEVEL_ALL = 0, LEVEL_PASS
	};
	enum ePathfindType
	{
		ASTAR = 0, JPS
	};

	struct Node
	{
		enum eNodeType
		{
			BLANK = 0, BLOCK, START, END, OPEN, CLOSE, PATH
		};
		Node* _parent;
		Node* _fixedparent;

		int   _X;
		int   _Y;
		int   _dir;
		int   _type;
		float _G;
		float _H;
		float _F;

		Node(void)
		{
			Clear();
		}

		void Clear(void)
		{
			_X = 0;
			_Y = 0;
			_G = 0;
			_H = 0;
			_F = 0;
			_dir = -1;
			_type = 0;
			_parent = nullptr;
			_fixedparent = nullptr;
		}
		struct cmp {
			bool operator()(Node* a, Node* b) const
			{
				return a->_F >= b->_F;
			}
		};
	};

	CPathFinder();
	virtual ~CPathFinder();

	void SetStart(int X, int Y)
	{
		Grid[Start._Y][Start._X] = nullptr;
		if (Grid[Y][X] != nullptr)
		{
			if (Grid[Y][X] != &Block && Grid[Y][X] != &End)
			{
				NodePool->Free(Grid[Y][X]);
			}
		}
		Grid[Y][X] = &Start;
	}
	void SetEnd(int X, int Y)
	{
		Grid[End._Y][End._X] = nullptr;
		if (Grid[Y][X] != nullptr)
		{
			if (Grid[Y][X] != &Block && Grid[Y][X] != &Start)
			{
				NodePool->Free(Grid[Y][X]);
			}
		}
		Grid[Y][X] = &End;
	}
	void SetBlock(int X, int Y)
	{
		if (Grid[Y][X] != nullptr && Grid[Y][X] != &Start && Grid[Y][X] != &End)
		{
			if (Grid[Y][X] != &Block)
			{
				NodePool->Free(Grid[Y][X]);
			}
		}
		Grid[Y][X] = &Block;
	}
	void SetBlank(int X, int Y)
	{
		if (Grid[Y][X] != nullptr && Grid[Y][X] != &Start && Grid[Y][X] != &End)
		{
			if (Grid[Y][X] != &Block)
			{
				NodePool->Free(Grid[Y][X]);
			}
		}
		Grid[Y][X] = nullptr;
	}
	void SetGForm(void)
	{
		if (iGform == EUCLIDEAN)
		{
			iGform = MANHATTAN;
		}
		else
		{
			iGform = EUCLIDEAN;
		}
	}

	void SetHForm(void)
	{
		if (iHform == EUCLIDEAN)
		{
			iHform = MANHATTAN;
		}
		else
		{
			iHform = EUCLIDEAN;
		}
	}
	int GetGridStatus(int X, int Y)
	{
		if (Grid[Y][X] == nullptr)
		{
			return Node::BLANK;
		}
		return Grid[Y][X]->_type;
	}
	int GetWidth(void)
	{
		return iMaxWidth;
	}
	int GetHeight(void)
	{
		return iMaxHeight;
	}
	void SetFixPathRun(void)
	{
		if (iFixPath == TRUE)
		{
			iFixPath = FALSE;
		}
		else
		{
			iFixPath = TRUE;
		}
	}

	void IncreaseGridSize(void)
	{
		GRID_SIZE++;
	}
	void DecreaseGridSize(void)
	{
		GRID_SIZE--;
	}
	int GetGridSize(void)
	{
		return GRID_SIZE;
	}

	void SetMapX(int x)
	{
		iScreenX += x;
	}
	void SetMapY(int y)
	{
		iScreenY += y;
	}
	void Initialize(int iMapWidth, int iMapHeight, int HWeight = 1, int Gform = EUCLIDEAN, int Hform = MANHATTAN);
	bool PathFindByStep(int sX, int sY, int eX, int eY);
	bool PathFindByStepUpdate(void);
	void PathFind(int sX, int sY, int eX, int eY);
	void Clear(void);
	void ClearBlock(void);
	virtual void ClearPath(void);

	void RenderGrid(HDC hdc); //그리드 배경
	void RenderText(HDC hdc); //설명
	virtual void Render(HDC hdc) = 0; //타일(장애물, 체크한 노드)



protected:

	virtual void OnLogic(Node* pNode) = 0;
	virtual void OnClear(void) = 0;
	virtual void OnPathFindComplete(void) = 0;
	virtual void OnPathFindFailed(void) = 0;

	float EuclideanDistance(int sX, int sY, int eX, int eY);
	int   ManhattanDistance(int sX, int sY, int eX, int eY);
	bool CheckBlockValid(int X, int Y);
	void FixPath(void); //bresenham
	bool CheckPathBlocked(Node* pStart, Node* pEnd);

	int   iType;
	int   iMaxWidth;
	int   iMaxHeight;
	int   iGform;
	int   iHform;
	float iHweight;
	int   iResult;
	int   iFixPath;

	int iScreenX;
	int iScreenY;

	Node*** Grid;
	Node Start;
	Node End;
	Node Block;

	std::priority_queue<Node*, std::vector<Node*>, Node::cmp> OpenList;
	CObjectFreeList<Node>* NodePool;

	//Render
	int iLevel;

	int GRID_SIZE = 20;
	HBRUSH hTileBrush;
	HBRUSH hFixedTileBrush;
	HBRUSH hCheckedTileBrush;

	HPEN hGridPen;
	HPEN hParentPen;

	HPEN hPathPen; //jps
	HPEN hFixedPathPen; //jps

	HBRUSH hStartBrush;
	HBRUSH hEndBrush;
	HBRUSH hOpenBrush;
	HBRUSH hCloseBrush;
	HBRUSH hPathBrush;

	bool bErase;
	bool bDrag;

	bool bClick;
	HPEN hPen;
	int iOldX;
	int iOldY;

};