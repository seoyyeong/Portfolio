#pragma once

#pragma comment(lib, "Synchronization.lib")
//#define __FREELIST_DEBUG

#ifdef __FREELIST_DEBUG
#define GUARDCODE 0xDFDFDFDF
#endif


template <typename T>

class CObjectFreeList
{
public : 
	enum eFreeList
	{
		MAX_POOLSIZE = 1000
	};

	enum LOCKTYPE
	{
		FREE = 0, EXCLUSIVE = 1, SHARED
	};

	struct Node
	{
#ifdef __FREELIST_DEBUG
		int _underflow;
#endif
		T _data;

#ifdef __FREELIST_DEBUG
		int _overflow;
#endif
		bool _isFree;
		int* _instanceCode;
		Node* _next;
	};

	struct NodeInfo
	{
		Node* _pNode;
		NodeInfo* _next;
	};

	class AllocList
	{
	public:

		AllocList()
		{
			_head = (NodeInfo*)malloc(sizeof(NodeInfo));
			_tail = (NodeInfo*)malloc(sizeof(NodeInfo));
			_head->_next = _tail;
			_tail->_next = nullptr;
			_size = 0;
		}

		~AllocList()
		{
			NodeInfo* pTemp;
			NodeInfo* pNode = _head;

			while (pNode != _tail)
			{
				pTemp = pNode;
				pNode = pNode->_next;
				free(pTemp);
			}
			free(_tail);
		}

		inline NodeInfo* GetFirst(void)
		{
			return _head->_next;
		}

		inline NodeInfo* GetTail(void)
		{
			return _tail;
		}

		inline Node* GetData(NodeInfo* pNode)
		{
			return pNode->_pNode;
		}

		void push(Node* pNode)
		{
			NodeInfo* newNode = (NodeInfo*)malloc(sizeof(NodeInfo));
			newNode->_pNode = pNode;
			newNode->_next = _head->_next;
			_head->_next = newNode;
			_size++;
		}



	private:
		NodeInfo* _head;
		NodeInfo* _tail;
		int _size;

	};

	CObjectFreeList(int iAllocSize = 100, char chPlacementNew = false)
	{
		Node* pPrev;
		Node* pNode;
		T* pData;

		_size = iAllocSize;
		_useSize = 0;
		_instanceID = (int*)this;
		_placementNew = chPlacementNew;

		ObjAllocList = new AllocList;

		///////////////////////////head allocation


		_head = (Node*)malloc(sizeof(Node));
		_head->_instanceCode = _instanceID;
		
		//////////////////////////data block allocation

		pPrev = _head;

		for (int i = 0; i < iAllocSize; i++)
		{
			pNode = (Node*)malloc(sizeof(Node));
#ifdef  __FREELIST_DEBUG
			pNode->_underflow = GUARDCODE;
			pNode->_overflow = GUARDCODE;
#else
#endif
			ObjAllocList->push(pNode);

			pNode->_instanceCode = _instanceID;
			pNode->_isFree = true;
			pPrev->_next = pNode;
			pData = &(pNode->_data);
			if (_placementNew == false)
				pData = new(pData) T;

			pPrev = pNode;

		}

		//////////////////////////tail allocation

		_tail = (Node*)malloc(sizeof(Node));
		_tail->_instanceCode = _instanceID;

		pPrev->_next = _tail;
		_tail->_next = nullptr;

	}

	virtual ~CObjectFreeList()
	{
		NodeInfo* iter = ObjAllocList->GetFirst();
		Node* pNode;

		while (iter != ObjAllocList->GetTail())
		{
			pNode = ObjAllocList->GetData(iter);

#ifdef  __FREELIST_DEBUG

			if (pNode->_underflow != GUARDCODE || pNode->_overflow != GUARDCODE)
			{
				int* crash = nullptr;
				crash = 0;
			}

#endif 
			if (_placementNew == false)
				pNode->_data.~T();
			else
			{
				if (pNode->_isFree == false)
					pNode->_data.~T();
			}


			free(pNode);

			iter = iter->_next;
		}

		free(_head);
		free(_tail);
		delete ObjAllocList;
	}

	T* Alloc(void)
	{
		Node* pNode;
		T*	  pData;

		if (_size >= MAX_POOLSIZE)
			return nullptr;

		if (_size == _useSize) //�� ���� ��� ó������ ���Ҵ�
		{

			pNode = (Node*)malloc(sizeof(Node));

#ifdef  __FREELIST_DEBUG
			pNode->_underflow = GUARDCODE;
			pNode->_overflow = GUARDCODE;
#endif
			pNode->_instanceCode = _instanceID;
			pData = &(pNode->_data);
			pData = new(pData) T;

			ObjAllocList->push(pNode);
			_size++;

			pNode->_isFree = false;

		}
		else //�ִ°� pop
		{
			pData = &(_head->_next->_data);

			_head->_next->_isFree = false;

			if (_placementNew == true) //������ ȣ���ؾ� �ϴ� ���
				pData = new(pData) T;
			
			_head->_next = _head->_next->_next;

		}

		_useSize++;
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

		pNode = (Node*)((char*)pData - ((int) & ((Node*)nullptr)->_data));

#ifdef  __FREELIST_DEBUG

		if (pNode->_underflow != GUARDCODE || pNode->_overflow != GUARDCODE)
		{
			int* crash = nullptr;
			crash = 0;
		}
#endif

		if (pNode->_instanceCode != _instanceID)
			return false;

		if (_placementNew == true)
			pNode->_data.~T();

		pNode->_next = _head->_next;
		_head->_next = pNode;

		pNode->_isFree = true;
		_useSize--;

		return true;

	}


	inline int GetPoolSize(void)
	{
		return _size;
	}

	inline int GetAllocSize(void)
	{
		return _useSize;
	}
	void Lock(int LockType)
	{
		if (LockType == SHARED)
		{
			AcquireSRWLockShared(&SRWLock);
		}
		else
		{
			AcquireSRWLockExclusive(&SRWLock);
		}
	}
	void UnLock(void)
	{
		if (SRWLock.Ptr == (void*)FREE)
		{
			return;
		}
		if (SRWLock.Ptr == (void*)EXCLUSIVE)
		{
			ReleaseSRWLockExclusive(&SRWLock);
			return;
		}

		ReleaseSRWLockShared(&SRWLock);
	}
private:
	Node* _head;
	Node* _tail;

	int _size;
	int _useSize;
	int* _instanceID;
	bool _placementNew;

	SRWLOCK SRWLock;

	AllocList* ObjAllocList;
};


