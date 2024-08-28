#pragma once
#include <windows.h>
#include "CCrashDump.h"
//#define __FREELIST_DEBUG

//오브젝트 프리 리스트는 2겹으로 되어 있습니다.
//실제로 Allocation 할 수 있는 노드의 리스트를 관리하는 ObjectFreeList는 _Top 변수로 관리하고,
//Pool의 Allocation 여부에 관계 없이 malloc으로 할당받은 노드를 모두 관리하는 배열 AllocArr를 하나 둡니다.

//#define __FREELIST_LOG

#ifdef __FREELIST_DEBUG
#define GUARDCODE 0xDFDFDFDF
#endif
//////////////CAS 관련 상수
#define SHIFT    47
#define KEY_MASK   0x00007FFF'FFFFFFFF
#define INDEX_MASK 0xFFFF8000'00000000

#ifdef __FREELIST_LOG
#define LOG_MAX 30'000'000

enum FUNCTYPE
{
	PUSH = 0, POP, PUSH_FAIL, POP_FAIL, MOVETAIL = 5
};
struct QueueLog
{
	int			idx;			//로그 인덱스
	char		chType;			//연산의 타입(push 0, pop 1)
	short		iThreadID;		//push/pop 연산을 수행한 스레드 ID
	UINT_PTR    pHead;			//연산 수행 대상 노드
	UINT_PTR    pTail;			//연산 수행 대상 노드
	UINT_PTR    pNext;			//연산 수행 대상 노드
};
struct Log
{
	int			idx;			//로그 인덱스
	char		chType;			//연산의 타입(push 0, pop 1)
	short		iThreadID;		//push/pop 연산을 수행한 스레드 ID
	UINT_PTR    pNode;			//연산 수행 대상 노드
	UINT_PTR    pNext;			//연산 수행 대상 노드
};

extern Log g_FreeListLog[LOG_MAX];
extern int iFreeLogCount = -1;
#endif

template <typename T>

class CLockFreeList
{
public:
	enum eFreeList
	{
		MAX_POOLSIZE = 150'000
	};

	struct Node
	{
#ifdef __FREELIST_DEBUG
		UINT_PTR _underflow;
#endif
		T _data;

#ifdef __FREELIST_DEBUG
		UINT_PTR _overflow;
#endif
		Node*   _next;
		Node*   _NextTotal;
		bool    _isFree;		//소멸자에서 delete여부 판단하기 위한 변수
		CLockFreeList* _instanceCode;  //메모리풀 객체 구분용
	};

	CLockFreeList(int iAllocSize = 100, char chPlacementNew = false)
	{
		//SYSTEM_INFO Info;
		//GetSystemInfo(&Info);
		//if (CASNUM > (__int64)Info.lpMaximumApplicationAddress)
		//{
		//	wprintf(L"INVALID CAS(MAX USER ADDRESS CHANGED)\n");
		//}

		Node* pNode;
		T*    pData;

		_size = 0;
		_useSize      = 0;
		_placementNew = chPlacementNew;

		_top  = 0;
		_TopTotal = 0;

		//////////////////////////data block allocation

		for (int i = 0; i < iAllocSize; i++)
		{
			pNode = (Node*)_aligned_malloc(sizeof(Node), 64);
#ifdef  __FREELIST_DEBUG
			pNode->_underflow = GUARDCODE;
			pNode->_overflow = GUARDCODE;
#else
#endif
			pNode->_instanceCode = this;
			pNode->_isFree = true;
			pData = &(pNode->_data);

			if (_placementNew == false)
			{
				pData = new(pData) T;
			}
			
			Push(pNode);
			PushTotal(pNode);

		}

	}

	virtual ~CLockFreeList()
	{
		Node* pNode;

		while (pNode = PopTotal())
		{

#ifdef  __FREELIST_DEBUG

			if (pNode->_underflow != GUARDCODE || pNode->_overflow != GUARDCODE)
			{
				int* crash = nullptr;
				crash = 0;
			}

#endif 
			if (_placementNew == false)
			{
				pNode->_data.~T();
			}

			else
			{
				if (pNode->_isFree == false)
				{
					pNode->_data.~T();
				}
			}

			_aligned_free(pNode);
		}

	}

	T* Alloc(void)
	{
		Node* pTemp;

		T* pData;
		pTemp = Pop();

		if (pTemp == nullptr)
		{
			if (InterlockedCompareExchange((long*)&_useSize, (long)_useSize, MAX_POOLSIZE) == MAX_POOLSIZE)
			{
				return nullptr;
			}

			pTemp = (Node*)_aligned_malloc(sizeof(Node), 64);

#ifdef  __FREELIST_DEBUG
			pTemp->_underflow = GUARDCODE;
			pTemp->_overflow = GUARDCODE;
#endif
			pTemp->_instanceCode = this;
			pTemp->_isFree = false;

			pData = &(pTemp->_data);
			pData = new(pData) T;

			PushTotal(pTemp);
		}
		else
		{

			pData = &(pTemp->_data);

			if (_placementNew == true)  //생성자 호출해야 하는 경우
			{
				pData = new(pData) T;
			}
			pData = &(pTemp->_data);
		}

		InterlockedIncrement((long*)&_useSize);
		return pData;
	}

	/// Free 
	/// 역할이 끝난 객체 포인터를 Object Free List로 반환
	/// 객체에 할당되었던 메모리를 실제로 해제하는 것은 아니고, 메모리풀 형식으로 관리합니다.
	/// 
	/// 매개변수 : T*(할당 해제할 포인터)
	/// 리턴 : false(현재 오브젝트풀의 객체가 아닐 경우), true(반환 성공)
	bool Free(T* pData)
	{
		Node* pNode;

		pNode = (Node*)((char*)pData - ((UINT_PTR)&((Node*)nullptr)->_data));

#ifdef  __FREELIST_DEBUG

		if (pNode->_underflow != GUARDCODE || pNode->_overflow != GUARDCODE)
		{
			int* crash = nullptr;
			crash = 0;
		}
#endif
		
		if (pNode->_instanceCode != this) //현재 객체에서 할당한 오브젝트가 아닐 경우 반환 실패
			return false;

		pNode->_isFree = true;

		if (_placementNew == true)
			pNode->_data.~T();

		Push(pNode);

		InterlockedDecrement((long*)&_useSize);
		return true;

	}


	inline int GetPoolSize(void) // 전체 pool size 반환
	{
		return _size;
	}

	inline int GetAllocSize(void)  // Alloc size 반환
	{
		return _useSize;
	}

	///////////////////////////////////////////////////////////////////////
	static inline bool CAS(__int64* dest, __int64 exchange, __int64 cmp)
	{

		if (InterlockedCompareExchange64(dest, exchange, cmp) == cmp)
		{
			return true;
		}
		return false;

	}

	static inline void* KeytoNode(__int64 iKey)
	{
		return (void*)(KEY_MASK & iKey);
	}

	static inline __int64 NodetoKey(void* pNode, __int64 idx)
	{
		return (idx << SHIFT) | (__int64)pNode;
	}

	static inline __int64 KeytoIndex(__int64 iKey)
	{
		return (iKey >> SHIFT);
	}
private:

	bool Push(Node* pNewNode)
	{
		__int64 iTempKey;
		__int64 iNewNodeKey;

		while (1)
		{
			iTempKey = _top;
			pNewNode->_next = (Node*)KeytoNode(iTempKey);
			iNewNodeKey = NodetoKey(pNewNode, KeytoIndex(iTempKey));

			if (CAS(&_top, iNewNodeKey, iTempKey))
			{
#ifdef __FREELIST_LOG
				unsigned int idx;
				idx = InterlockedIncrement((long*)&iFreeLogCount);
				idx %= LOG_MAX;
				g_FreeListLog[idx].chType = PUSH;
				g_FreeListLog[idx].iThreadID = (short)GetThreadId(GetCurrentThread());
				g_FreeListLog[idx].pNode = (UINT_PTR)iNewNodeKey;
				g_FreeListLog[idx].pNext = (UINT_PTR)iTempKey;
#endif
				return true;
			}
		}
	}

	Node* Pop(void)
	{
		Node* pTemp;
		__int64 iTempKey;
		__int64 iNextKey;

		while (1)
		{
			iTempKey = _top;
			pTemp    = (Node*)KeytoNode(iTempKey);

			if (pTemp == nullptr)
			{
				return nullptr;
			}

			iNextKey = NodetoKey(pTemp->_next, KeytoIndex(iTempKey) + 1);

			if (CAS(&_top, iNextKey, iTempKey))
			{
#ifdef __FREELIST_LOG
				unsigned int idx;
				idx = InterlockedIncrement((long*)&iFreeLogCount);
				idx %= LOG_MAX;
				g_FreeListLog[idx].chType = POP;
				g_FreeListLog[idx].iThreadID = (short)GetThreadId(GetCurrentThread());
				g_FreeListLog[idx].pNode = (UINT_PTR)iNextKey;
				g_FreeListLog[idx].pNext = (UINT_PTR)iTempKey;
#endif
				return pTemp;
			}

		}

	}

	bool PushTotal(Node* pNewNode)
	{
		__int64 iTempKey;
		__int64 iNewNodeKey;

		while (1)
		{
			iTempKey = _TopTotal;
			pNewNode->_NextTotal = (Node*)KeytoNode(iTempKey);
			iNewNodeKey = NodetoKey(pNewNode, KeytoIndex(iTempKey));

			if (CAS(&_TopTotal, iNewNodeKey, iTempKey))
			{
				InterlockedIncrement((long*)&_size);
				return true;
			}
		}
	}

	Node* PopTotal(void)
	{
		Node* pTemp;
		__int64 iTempKey;
		__int64 iNextKey;

		while (1)
		{
			iTempKey = _TopTotal;
			pTemp = (Node*)KeytoNode(iTempKey);

			if (pTemp == nullptr)
			{
				return nullptr;
			}

			iNextKey = NodetoKey(pTemp->_NextTotal, KeytoIndex(iTempKey) + 1);

			if (CAS(&_TopTotal, iNextKey, iTempKey))
			{
				InterlockedDecrement((long*)&_size);
				return pTemp;
			}
		}

	}
	alignas(64) __int64 _top;
	__int64 _TopTotal;

	unsigned int  _size;         //총 메모리풀 카운터
	unsigned int  _useSize;  //Alloc 완료된 메모리 카운터

	bool _placementNew; //placement new 여부
};


