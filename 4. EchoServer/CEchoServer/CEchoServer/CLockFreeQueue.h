#pragma once

#include "CTlsPool.h"

template <typename T>

class CLockFreeQueue
{
private:

    struct Node
    {
        T data;
        Node* next;
    };

    alignas(64) __int64 _head;
    alignas(64) __int64 _tail;

    //alignas(64) static CTlsPool<Node>* FreeList;
    alignas(64) CLockFreeList<Node>* FreeList;
    long _size;

    inline bool CAS(__int64* dest, __int64 exchange, __int64 cmp)
    {    
        if (InterlockedCompareExchange64(dest, exchange, cmp) == cmp)
        {
            return true;
        }
        return false;
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

public:
    CLockFreeQueue(int iSize = 128)
    {
        Node* pNode;

        //FreeList = new CTlsPool<Node>(3);
        FreeList = new CLockFreeList<Node>(iSize);
        pNode = FreeList->Alloc();
        pNode->next = NULL;

        _head = NodetoKey(pNode, 0);
        _tail = _head;

        _size = 0;
    }

    ~CLockFreeQueue()
    {
        T t;
        while (_size > 0)
        {
            Dequeue(t);
        }
        delete FreeList;
    }

    inline long Size(void)
    {
        return InterlockedOr(&_size, 0);
    }

    void Clear(void)
    {
        T t;
        while (_size > 0)
        {
            Dequeue(t);
        }
    }

    bool Enqueue(T t) //tail 쪽으로 enqueue
    {
        __int64 iTailKey;
        __int64 iNewNodeKey;
        __int64 iNextKey;
        Node*   pNewNode;
        Node*   pNext;
        Node*   pTail;

        pNewNode = FreeList->Alloc();

        if (pNewNode == nullptr)
        {
            return false;
        }

        pNewNode->data = t;
        pNewNode->next = NULL;


        while (true)
        {
            iTailKey = _tail;
            pTail = KeytoNode(iTailKey);
            pNext = pTail->next;

            iNewNodeKey = NodetoKey(pNewNode, KeytoIndex(iTailKey) + 1);

            if (pNext == nullptr)
            {
                if (CAS((__int64*)&pTail->next, (__int64)pNewNode, (__int64)pNext))
                {
                    CAS(&_tail, iNewNodeKey, iTailKey);
                    break;
                }
            }
            
            else
            {
                iNextKey = NodetoKey(pNext, KeytoIndex(iTailKey) + 1);
                CAS(&_tail, iNextKey, iTailKey);
            }
            
        }

        InterlockedIncrement(&_size);
        return true;
    }

    int Dequeue(T& t) //head 쪽에서 dequeue
    {
        __int64 iHeadKey;
        __int64 iNextKey;
        __int64 iTailKey;
        __int64 iTailNextKey;

        Node* pNext;
        Node* pHead;
        Node* pTail;
        Node* pTailNext;

        T data;

        int size = InterlockedAdd(&_size, -1);
        if (_size < 0)
        {
            InterlockedIncrement(&_size);
            return FALSE;
        }

        while (true)
        {
            iHeadKey = _head;
            pHead = KeytoNode(iHeadKey);
            pNext = pHead->next;
            iNextKey = NodetoKey(pNext, KeytoIndex(iHeadKey) + 1);

            iTailKey = _tail;
            pTail = KeytoNode(iTailKey);
            pTailNext = pTail->next;
            iTailNextKey = NodetoKey(pTailNext, KeytoIndex(iTailKey) + 1);

            if (pTailNext != nullptr)
            {
                CAS(&_tail, iTailNextKey, iTailKey);
            }

            else if (pNext == nullptr)
            {
                continue;
            }

            else
            {
                data = pNext->data;

                if (CAS(&_head, iNextKey, iHeadKey))
                {
                    t = data;
                    FreeList->Free(pHead);

                    break;
                }
            }
        }

        return TRUE;
    }
};