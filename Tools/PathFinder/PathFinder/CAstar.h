#pragma once

#include "CPathFinder.h"

class CAstar : public CPathFinder
{
public:
	CAstar()
	{
		iType = ASTAR;
	}
	void Render(HDC hdc);
	void Initialize(void);

private:
	void OnLogic(Node* pNode);
	void OnClear(void){}
	void OnPathFindComplete(void);
	void OnPathFindFailed(void) {}

	Node* CreateNode(Node* pParent, int Direction);

};