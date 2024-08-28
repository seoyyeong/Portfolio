#pragma once
//#include <iostream>
//#include <Windows.h>
#include "pch.h"

class CRingBuffer
{
private:

	enum BUFSIZE
	{
		MAX_BUFSIZE = 512
	};


	char* buf;
	char* front;
	char* rear;
	char* bufend;
	int   BufSize;

	SRWLOCK stEnqueueLock;
	SRWLOCK stDequeueLock;
	SRWLOCK stSRWLock;

public:
	enum LOCKTYPE
	{
		FREE = 0, EXCLUSIVE = 1, SHARED
	};

	CRingBuffer(void);
	CRingBuffer(int Size);
	~CRingBuffer(void);

	int Resize(void);

	inline int GetBufSize(void)
	{
		return BufSize - 1;
	}
	inline int GetUsedSize(void)
	{
		if (front <= rear)
		{
			return (int)(rear - front);
		}
		else
		{
			return (int)((bufend - front) + (rear - buf));
		}

	}
	inline int GetAvailableSize(void)
	{
		if (front > rear)
		{
			return (int)(front - rear - 1);
		}
		else
		{
			return (int)((bufend - rear) + (front - buf) - 1);
		}
	}


	inline int DirectEnqueueSize(void)
	{
		char* front = this->front;

		if (front > rear)
		{
			return (int)(front - rear - 1);
		}
		else
		{
			if (front == buf)
			{
				return (int)(bufend - rear - 1);
			}
			else
			{
				return (int)(bufend - rear);
			}

		}
		//int size = BufSize - 1;

		//if (front > rear)
		//{
		//	size = front - rear - 1;
		//}
		//else if (front < rear)
		//{
		//	size = bufend - rear;
		//}

		//return size;
	}
	inline int DirectDequeueSize(void)
	{
		char* rear = this->rear;

		if (front <= rear)
		{
			return (int)(rear - front);
		}
		else if (front > rear)
		{
			return (int)(bufend - front);
		}

		return 0;
	}

	inline int Enqueue(const char* pData, int Size)
	{
		int EnqueueSize = DirectEnqueueSize();

		if (Size <= EnqueueSize)
			memcpy(rear, pData, Size);

		else
		{
			memcpy(rear, pData, EnqueueSize);
			memcpy(buf, pData + EnqueueSize, Size - EnqueueSize);
		}

		MoveRear(Size);

		return Size;
	}
	inline int Dequeue(char* pDest, int Size)
	{
		int DequeueSize = DirectDequeueSize();

		if (Size <= DequeueSize)
			memcpy(pDest, front, Size);

		else
		{
			memcpy(pDest, front, DequeueSize);
			memcpy(pDest + DequeueSize, buf, Size - DequeueSize);
		}

		MoveFront(Size);

		return Size;
	}
	inline int Peek(char* pDest, int Size)
	{
		int DequeueSize = DirectDequeueSize();


		if (Size <= DequeueSize)
			memcpy(pDest, front, Size);

		else
		{
			memcpy(pDest, front, DequeueSize);
			memcpy(pDest + DequeueSize, buf, Size - DequeueSize);
		}

		return Size;

	}

	inline UINT_PTR MoveRear(int Size)
	{
		rear = rear + Size;
		if (rear >= bufend)
		{
			UINT_PTR move = rear - bufend;
			rear = buf + move;
		}

		return Size;
	}
	inline UINT_PTR MoveFront(int Size)
	{
		front = front + Size;
		if (front >= bufend)
		{
			UINT_PTR move = front - bufend;
			front = buf + move;
		}

		return Size;
	}


	inline void ClearBuffer(void)
	{
		front = rear;
	}
	inline char* GetBuf(void)
	{
		return buf;
	}
	inline char* GetBufRear(void)
	{
		return rear;
	}
	inline char* GetBufFront(void)
	{
		return front;
	}


	void Lock(int LockType)
	{
		if (LockType == SHARED)
		{
			AcquireSRWLockShared(&stSRWLock);
		}
		else
		{
			AcquireSRWLockExclusive(&stSRWLock);
		}
	}
	void UnLock(void)
	{
		if (stSRWLock.Ptr == (void*)FREE)
		{
			return;
		}
		if (stSRWLock.Ptr == (void*)EXCLUSIVE)
		{
			ReleaseSRWLockExclusive(&stSRWLock);
			return;
		}

		ReleaseSRWLockShared(&stSRWLock);
	}

	void EnqueueUnLock(void)
	{
		if (&stEnqueueLock.Ptr == (void*)FREE)
		{
			return;
		}
		if (&stEnqueueLock.Ptr == (void*)EXCLUSIVE)
		{
			ReleaseSRWLockExclusive(&stEnqueueLock);
			return;
		}

		ReleaseSRWLockShared(&stEnqueueLock);
	}

	void DequeueLock(int LockType)
	{
		if (LockType == SHARED)
		{
			AcquireSRWLockShared(&stDequeueLock);
		}
		else
		{
			AcquireSRWLockExclusive(&stDequeueLock);
		}
	}
	void DequeueUnLock(void)
	{
		if (&stDequeueLock.Ptr == (void*)FREE)
		{
			return;
		}
		if (&stDequeueLock.Ptr == (void*)EXCLUSIVE)
		{
			ReleaseSRWLockExclusive(&stDequeueLock);
			return;
		}

		ReleaseSRWLockShared(&stDequeueLock);
	}
};