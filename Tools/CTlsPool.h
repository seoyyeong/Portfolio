#pragma once

#include "pch.h"

#define MAX_BUCKETSIZE 512


template <typename T>

class CTlsPool
{

public:

	struct Node
	{
		T        _data;
		Node*    _next;
	};

	class CBucket
	{
		friend CTlsPool;

	public:

		inline CBucket()
		{
			_TopNode = nullptr;
			_Mid = nullptr;
			_Next = nullptr;
			_iSize = MAX_BUCKETSIZE;
		}

	private:

		Node*    _TopNode;
		Node*    _Mid;

		CBucket* _Next;
		int      _iSize;    //MAX_BUCKETSIZE * 2��ŭ ���� ����

	};

	CTlsPool(int iBlockSize = 10, bool bPlacementNew = false)
	{
		_dwTlsIndex = TlsAlloc();

		if (_dwTlsIndex == TLS_OUT_OF_INDEXES)
		{
			CRASH();
		}
		
		//0. ������� �ʱ�ȭ
		_Top = 0;
		_iSize = 0;
		_iNodeSize = iBlockSize * MAX_BUCKETSIZE;
		_iMaxNode = _iNodeSize;
		_bPlacementNew = bPlacementNew;

		//1. ������ƮǮ �ʱ�ȭ : ���ʿ� ��� ���� ��Ŷ �� ��� �Ҵ�
		_BucketPool = new CLockFreeList<CBucket>(iBlockSize * 2, true); //��Ŷ�� *2��ŭ �Ҵ�, �� ��Ŷ�� �ʿ��� ��Ȳ�� �����Ƿ�
		_NodePool = new CLockFreeList<Node>(_iNodeSize);

		//2. ���� ��Ŷ ����
		for (int i = 0; i < iBlockSize; i++)
		{
			CBucket* pBucket = _BucketPool->Alloc();

			for (int j = 0; j < MAX_BUCKETSIZE; j++)
			{
				Node* pNode = _NodePool->Alloc();
				pNode->_next = pBucket->_TopNode;
				pBucket->_TopNode = pNode;
				
			}
			PushBucket(pBucket);
		}

	}

	~CTlsPool()
	{
		TlsFree(_dwTlsIndex);

		delete _BucketPool;
		delete _NodePool;

	}

	T* Alloc()
	{
		CBucket* pBucket;
		T* pData;

		pBucket = (CBucket*)TlsGetValue(_dwTlsIndex);

		if (pBucket == nullptr)
		{
			pBucket = PopBucket();
			TlsSetValue(_dwTlsIndex, pBucket);
		}

		pData = (T*)pBucket->_TopNode;
		if (pData == nullptr)
		{
			if (_iSize == 0)
			{
				CRASH();
			}
			CBucket* pTop = PopBucket();

			pBucket->_TopNode = pTop->_TopNode;
			pBucket->_iSize = MAX_BUCKETSIZE;
		    pData = (T*)pBucket->_TopNode;

			_BucketPool->Free(pTop);	
		}

		pBucket->_TopNode = pBucket->_TopNode->_next;
		pBucket->_iSize--;
		InterlockedDecrement((long*)&_iNodeSize);

		if (_bPlacementNew)
		{
			pData = new(pData) T;
		}

		return pData;
	}

	bool Free(T* pData)
	{

		CBucket* pBucket;
		pBucket = (CBucket*)TlsGetValue(_dwTlsIndex);

		if (pBucket == nullptr)
		{
			pBucket = PopBucket();
			TlsSetValue(_dwTlsIndex, pBucket);
		}

		else
		{
			if (pBucket->_iSize >= MAX_BUCKETSIZE * 2)
			{
				pBucket->_iSize = MAX_BUCKETSIZE;

				CBucket* pNewTop = _BucketPool->Alloc();
				pNewTop->_TopNode = pBucket->_Mid->_next;
				pBucket->_Mid->_next = nullptr;

				PushBucket(pNewTop);
			}
		}


		Node* pNode = (Node*)pData;

		if (pBucket->_iSize == MAX_BUCKETSIZE)
		{
			pBucket->_Mid = pNode;
		}

		pNode->_next = pBucket->_TopNode;
		pBucket->_TopNode = pNode;
		pBucket->_iSize++;
		InterlockedIncrement((long*)&_iNodeSize);

		if (_bPlacementNew)
		{
			pData->~T();
		}

		return true;
	}

	inline int GetSize(void)
	{
		return _iNodeSize;
	}

	inline int GetTotalSize(void)
	{
		return _iMaxNode;
	}

	inline int GetAllocSize(void)
	{
		return _iMaxNode - _iNodeSize;
	}

private:

	void PushBucket(CBucket* pBucket)
	{
		__int64 iTempKey;
		__int64 iNewKey;

		while (1)
		{
			iTempKey = _Top;
			pBucket->_Next = (CBucket*)CLockFreeList<CBucket>::KeytoNode(iTempKey);
			iNewKey = ((iTempKey & INDEX_MASK) | (__int64)pBucket);

			if (InterlockedCompareExchange64(&_Top, iNewKey, iTempKey) == iTempKey)
			{
				InterlockedIncrement((long*)&_iSize);
				return;
			}
		}
	}

	CBucket* PopBucket(void)
	{
		CBucket* pTemp;
		__int64 iTempKey;
		__int64 iNextKey;

		while (1)
		{
			iTempKey = _Top;
			pTemp = (CBucket*)CLockFreeList<CBucket>::KeytoNode(iTempKey);

			if (pTemp == nullptr)
			{
				return nullptr;
			}
			iNextKey = CLockFreeList<CBucket>::NodetoKey(pTemp->_Next, CLockFreeList<CBucket>::KeytoIndex(iTempKey) + 1);

			if (InterlockedCompareExchange64(&_Top, iNextKey, iTempKey) == iTempKey)
			{
				InterlockedDecrement((long*)&_iSize);
				return pTemp;
			}

		}
	}

	__int64	  _Top;        //�Ҵ� �غ� �Ϸ�� ��Ŷ ����Ʈ
	char       padding1[64];
	int       _iSize;
	int       _iMaxNode;
	int       _iNodeSize;

	char	   padding2[64];
	CLockFreeList<CBucket>* _BucketPool; //��Ŷ �����⸸ ������ �ִ� ������ƮǮ
	CLockFreeList<Node>*    _NodePool;
	DWORD				    _dwTlsIndex;
	bool					_bPlacementNew;

};