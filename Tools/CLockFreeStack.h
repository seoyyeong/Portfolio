#pragma once

#include "CTlsPool.h"

template <typename T>

class CLockFreeStack
{

public:

	struct Node
	{
		T     _data;
		Node* _Next;
	};

private:
	inline bool CAS(__int64* dest, __int64 exchange, __int64 cmp)
	{
		if (InterlockedCompareExchange64(dest, exchange, cmp) == cmp)
		{
			return TRUE;
		}
		return FALSE;

	}

	inline Node* KeytoNode(__int64 iKey)
	{
		return (Node*)(KEY_MASK & iKey);
	}

	inline __int64 NodetoKey(Node* pNode, __int64 idx)
	{
		return (idx << SHIFT) | (__int64)pNode;
	}

	inline __int64 KeytoIndex(__int64 iKey)
	{
		return (iKey >> SHIFT);
	}


	alignas(64) CLockFreeList<Node>* ObjectList;
	alignas(64) __int64 _Top;
	alignas(64) Node    _Tail;
	int     _size;

public:

	CLockFreeStack(int iSize = 100)
	{
		///getsysteminfo 확인함수 추가
		_Tail._Next = nullptr;
		_Top = NodetoKey(&_Tail, 0);
		ObjectList = new CLockFreeList<Node>(iSize);

		_size = 0;

	}

	~CLockFreeStack()
	{
		Clear();
		delete ObjectList;
	}

	inline int Size()
	{
		return _size;
	}

	inline bool isEmpty()
	{
		return _size == 0;
	}
	int GetAllocSize()
	{
		return ObjectList->GetAllocSize();
	}
	void Clear()
	{
		T t;
		while (_size > 0)
		{
			Pop(t);
		}

	}

	bool Push(T pData)
	{
		__int64 iTempKey;
		__int64 iNewNodeKey;
		Node* pNewNode;

		pNewNode = ObjectList->Alloc();
		
		if (pNewNode == nullptr)
		{
			return FALSE;
		}
		 
		pNewNode->_data = pData;

		while (1)
		{
			iTempKey = _Top;
			pNewNode->_Next = KeytoNode(iTempKey);
			iNewNodeKey = NodetoKey(pNewNode, KeytoIndex(iTempKey) + 1);

			if (CAS(&_Top, iNewNodeKey, iTempKey))
			{
				InterlockedIncrement((long*)&_size);
				return TRUE;
			}

		}
	}

	bool Pop(T& t)
	{
		Node*    pTemp;
		T        Data;
		__int64 iTempKey;
		__int64 iNextKey;

		while (1)
		{
			iTempKey = _Top;
			pTemp = KeytoNode(iTempKey);

			if (pTemp == &_Tail)
			{
				return FALSE;
			}

			iNextKey = NodetoKey(pTemp->_Next, KeytoIndex(iTempKey));
 			Data = pTemp->_data;

			if (CAS(&_Top, iNextKey, iTempKey))
			{
				ObjectList->Free(pTemp);
				InterlockedDecrement((long*)&_size);
				t = Data;

				return TRUE;
			}

		}

	}

};