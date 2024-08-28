#pragma once
#include "pch.h"
#pragma comment(lib, "lib/mysqlclient.lib")

#include "include/mysql.h"
#include "include/errmsg.h"



class CDBConnector
{
public:

	CDBConnector()
	{
		InitializeSRWLock(&DBLock);
		bIsAlive = FALSE;
	}

	~CDBConnector()
	{
		if (InterlockedOr8((char*)&bIsAlive, FALSE) != FALSE)
		{
			CloseConnection();
		}
	}

	int InitConnection(const WCHAR* DBIP, short port, const WCHAR* UserName, const WCHAR* Password, const WCHAR* DBName);

	int InitConnection(const char* DBIP, short port, const char* UserName, const char* Password, const char* DBName);

	void CloseConnection(void);

	bool BeginTransaction(void);

	bool EndTransaction(void);

	bool Execute(const char* pQuery);

	void StoreResult(void);

	void Fetch(MYSQL_ROW& Row);

	void FreeResult(void);

	bool GetRowCount(int& iCount);

	void Lock(void)
	{
		AcquireSRWLockExclusive(&DBLock);
	}

	void UnLock(void)
	{
		ReleaseSRWLockExclusive(&DBLock);
	}

	__inline MYSQL* GetConnection(void)
	{
		return connection;
	}

private:

	bool   bIsAlive;
	MYSQL  conn;
	MYSQL* connection;
	MYSQL_RES* Result;
	SRWLOCK    DBLock;
};

