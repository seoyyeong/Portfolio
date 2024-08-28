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

		if (_size == _useSize) //다 썼을 경우 처음부터 재할당
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
		else //있는거 pop
		{
			pData = &(_head->_next->_data);

			_head->_next->_isFree = false;

			if (_placementNew == true) //생성자 호출해야 하는 경우
				pData = new(pData) T;
			
			_head->_next = _head->_next->_next;

		}

		_useSize++;
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


