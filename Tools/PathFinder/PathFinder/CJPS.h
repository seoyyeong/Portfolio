#pragma once
#include "CPathFinder.h"

class CJPS :public CPathFinder
{
public:
	struct coord
	{
		int _X;
		int _Y;
		float _G;

		bool operator!=(const coord& Param)
		{
			if (this->_X != Param._X || this->_Y != Param._Y || this->_G != Param._G)
			{
				return TRUE;
			}
			return FALSE;
		}
	};

	CJPS(void)
	{
		iType = JPS;
	}
	~CJPS()
	{
		for (int i = 0; i < iMaxHeight; i++)
		{
			delete[] Checked[i];
		}
		delete[] Grid;
	}

	void Initialize(void);

	void Render(HDC hdc);

private:
	void OnLogic(Node* pNode);
	void OnClear(void);
	void OnPathFindComplete(void);
	void OnPathFindFailed(void) {}

	Node* CreateNode(Node* pParent, int X, int Y, int G, int Direction);

	inline bool CheckPath(Node* pNode, int dir);
	inline bool CheckStraight(Node* pNode, int X, int Y, int G, int dir, coord& Coord);
	inline bool CheckDiagonal(Node* pNode, int dir, coord& Coord);


	int iCheckedIdx;
	int** Checked;
	std::vector<std::vector<int>> ColorList;

};