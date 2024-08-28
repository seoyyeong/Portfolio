#pragma once
#include <windows.h>
#include "CCrashDump.h"
//#define __FREELIST_DEBUG

//������Ʈ ���� ����Ʈ�� 2������ �Ǿ� �ֽ��ϴ�.
//������ Allocation �� �� �ִ� ����� ����Ʈ�� �����ϴ� ObjectFreeList�� _Top ������ �����ϰ�,
//Pool�� Allocation ���ο� ���� ���� malloc���� �Ҵ���� ��带 ��� �����ϴ� �迭 AllocArr�� �ϳ� �Ӵϴ�.

//#define __FREELIST_LOG

#ifdef __FREELIST_DEBUG
#define GUARDCODE 0xDFDFDFDF
#endif
//////////////CAS ���� ���
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
	int			idx;			//�α� �ε���
	char		chType;			//������ Ÿ��(push 0, pop 1)
	short		iThreadID;		//push/pop ������ ������ ������ ID
	UINT_PTR    pHead;			//���� ���� ��� ���
	UINT_PTR    pTail;			//���� ���� ��� ���
	UINT_PTR    pNext;			//���� ���� ��� ���
};
struct Log
{
	int			idx;			//�α� �ε���
	char		chType;			//������ Ÿ��(push 0, pop 1)
	short		iThreadID;		//push/pop ������ ������ ������ ID
	UINT_PTR    pNode;			//���� ���� ��� ���
	UINT_PTR    pNext;			//���� ���� ��� ���
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
		bool    _isFree;		//�Ҹ��ڿ��� delete���� �Ǵ��ϱ� ���� ����
		CLockFreeList* _instanceCode;  //�޸�Ǯ ��ü ���п�
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

			if (_placementNew == true)  //������ ȣ���ؾ� �ϴ� ���
			{
				pData = new(pData) T;
			}
			pData = &(pTemp->_data);
		}

		InterlockedIncrement((long*)&_useSize);
		return pData;
	}

	/// Free 
	/// ������ ���� ��ü �����͸� Object Free List�� ��ȯ
	/// ��ü�� �Ҵ�Ǿ��� �޸𸮸� ������ �����ϴ� ���� �ƴϰ�, �޸�Ǯ �������� �����մϴ�.
	/// 
	/// �Ű����� : T*(�Ҵ� ������ ������)
	/// ���� : false(���� ������ƮǮ�� ��ü�� �ƴ� ���), true(��ȯ ����)
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
		
		if (pNode->_instanceCode != this) //���� ��ü���� �Ҵ��� ������Ʈ�� �ƴ� ��� ��ȯ ����
			return false;

		pNode->_isFree = true;

		if (_placementNew == true)
			pNode->_data.~T();

		Push(pNode);

		InterlockedDecrement((long*)&_useSize);
		return true;

	}


	inline int GetPoolSize(void) // ��ü pool size ��ȯ
	{
		return _size;
	}

	inline int GetAllocSize(void)  // Alloc size ��ȯ
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

	unsigned int  _size;         //�� �޸�Ǯ ī����
	unsigned int  _useSize;  //Alloc �Ϸ�� �޸� ī����

	bool _placementNew; //placement new ����
};


