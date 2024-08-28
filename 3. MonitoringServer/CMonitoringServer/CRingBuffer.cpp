
#include "pch.h"
#include "CRingBuffer.h"


CRingBuffer::CRingBuffer(void)
{
	buf = (char*)malloc(MAX_BUFSIZE + 1);
	BufSize = MAX_BUFSIZE + 1;
	front = rear = buf;
	bufend = buf + BufSize;

	InitializeSRWLock(&stSRWLock);
	InitializeSRWLock(&stEnqueueLock);
	InitializeSRWLock(&stDequeueLock);
}

CRingBuffer::CRingBuffer(int Size)
{
	buf = (char*)malloc(Size + 1);
	BufSize = Size + 1;
	front = rear = buf;
	bufend = buf + BufSize;

	InitializeSRWLock(&stSRWLock);
	InitializeSRWLock(&stEnqueueLock);
	InitializeSRWLock(&stDequeueLock);
}

CRingBuffer::~CRingBuffer(void)
{
	free(buf);
}

int CRingBuffer::Resize()
{
	if (BufSize * 2 >= MAX_BUFSIZE)
		return 0;

	char* temp = buf;
	char* newBuf = (char*)malloc(BufSize * 2 + 1);
	Dequeue(newBuf, GetUsedSize());

	buf = newBuf;
	front = newBuf;
	rear = newBuf + GetUsedSize();
	BufSize = BufSize * 2 + 1;

	return BufSize;
}
