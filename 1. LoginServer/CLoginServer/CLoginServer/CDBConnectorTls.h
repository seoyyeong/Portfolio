#pragma once
#include "CDBConnector.h"

#define dfDBCONNTLS_MAX_POOLSIZE 64


class CDBConnectorTls
{
public:

	CDBConnectorTls(int iPoolSize = dfDBCONNTLS_MAX_POOLSIZE)
	{

		InitializeSRWLock(&DBLock);

		dwTlsIndex = TLS_OUT_OF_INDEXES;
		iDBConnectorCnt = 0;

		DBConnectorPool = new CLockFreeList<CDBConnector>(iPoolSize);
		dwTlsIndex = TlsAlloc();

		if (dwTlsIndex == TLS_OUT_OF_INDEXES)
		{
			CRASH();
		}


	}

	~CDBConnectorTls()
	{
		if (dwTlsIndex != TLS_OUT_OF_INDEXES)
		{
			TlsFree(dwTlsIndex);
		}

		for (int i = 0; i < iDBConnectorCnt; i++)
		{
			DBConnectorArr[i]->CloseConnection();
			DBConnectorPool->Free(DBConnectorArr[i]);
		}

		if (DBConnectorPool != nullptr)
		{
			delete DBConnectorPool;
		}
	}

	void InitConnection(const WCHAR* DBIP, short port, const WCHAR* UserName, const WCHAR* Password, const WCHAR* DBName);

	void CloseConnection(void);

	bool BeginTransaction(void);

	bool EndTransaction(void);

	bool Execute(const char* pQuery);

	void StoreResult(void);

	void Fetch(MYSQL_ROW& Row);

	void FreeResult(void);

	bool GetRowCount(int& iCount);


	__inline CDBConnector* GetConnector(void)
	{
		CDBConnector* pConnector;

		pConnector = (CDBConnector*)TlsGetValue(dwTlsIndex);

		if (pConnector == nullptr)
		{
			pConnector = DBConnectorPool->Alloc();

			Lock();
			if (pConnector->InitConnection(IP, Port, Name, Pw, DB) != 0)
			{
				DBConnectorPool->Free(pConnector);
				UnLock();
				return nullptr;
			}
			UnLock();

			int idx = InterlockedIncrement((long*)&iDBConnectorCnt) - 1;
			DBConnectorArr[idx] = pConnector;

			TlsSetValue(dwTlsIndex, pConnector);
		}
		return pConnector;
	}
	void Lock(void)
	{
		AcquireSRWLockExclusive(&DBLock);
	}

	void UnLock(void)
	{
		ReleaseSRWLockExclusive(&DBLock);
	}

private:
	CLockFreeList<CDBConnector>* DBConnectorPool;
	CDBConnector*      DBConnectorArr[dfDBCONNTLS_MAX_POOLSIZE];
	int                iDBConnectorCnt;
	DWORD		       dwTlsIndex;

	char  IP[20];
	char  Name[20];
	char  Pw[20];
	char  DB[20];
	short Port;

	SRWLOCK    DBLock;
};

