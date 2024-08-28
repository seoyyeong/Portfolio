#include "pch.h"
#include "CDBConnector.h"

int CDBConnector::InitConnection(const WCHAR* DBIP, short port, const WCHAR* UserName, const WCHAR* Password, const WCHAR* DBName)
{
	char IP[20];
	char Name[20];
	char Pw[20];
	char DB[20];
	size_t len;
	WCHAR  error[256];

	// 초기화
	mysql_init(&conn);

	wcstombs_s(&len, IP, DBIP, 20);
	wcstombs_s(&len, Name, UserName, 20);
	wcstombs_s(&len, Pw, Password, 20);
	wcstombs_s(&len, DB, DBName, 20);


	// DB 연결
	connection = mysql_real_connect(&conn, IP, Name, Pw, DB, port, (char*)NULL, 0);
	if (connection == NULL)
	{
		mbstowcs_s(&len, error, mysql_error(&conn), 256);
		_LOG(L"Query", CLog::LEVEL_ERROR, L"Mysql Connect error : %s\n", mysql_error(&conn));
	}

	InterlockedExchange8((char*)&bIsAlive, TRUE);
	return 0;
}

int CDBConnector::InitConnection(const char* DBIP, short port, const char* UserName, const char* Password, const char* DBName)
{
	WCHAR  error[256];
	size_t len;

	mysql_init(&conn);

	connection = mysql_real_connect(&conn, DBIP, UserName, Password, DBName, port, (char*)NULL, 0);
	if (connection == NULL)
	{
		mbstowcs_s(&len, error, mysql_error(&conn), 256);
		_LOG(L"Query", CLog::LEVEL_ERROR, L"Mysql Connect error : %s\n", error);
		return -1;
	}

	InterlockedExchange8((char*)&bIsAlive, TRUE);
	return 0;
}
void CDBConnector::CloseConnection(void)
{
	mysql_close(connection);
	InterlockedExchange8((char*)&bIsAlive, FALSE);
}

bool CDBConnector::BeginTransaction(void)
{
	return Execute("begin");
}

bool CDBConnector::EndTransaction(void)
{
	return Execute("commit");
}

bool CDBConnector::Execute(const char* pQuery)
{
	int    query_stat;
	WCHAR  error[256];
	size_t len;

	query_stat = mysql_query(connection, pQuery);
	for (int i = 0; i < 5; i++)
	{
		if (query_stat == 0)
		{
			return TRUE;
		}
		mbstowcs_s(&len, error, mysql_error(&conn), 256);
		_LOG(L"Query", CLog::LEVEL_ERROR, L"Mysql query error : %s\n", error);
		CRASH();
		query_stat = mysql_query(connection, "SHOW DATABASES");
	}
	return FALSE;
}

void CDBConnector::StoreResult(void)
{
	Result = mysql_store_result(connection);
}

void CDBConnector::Fetch(MYSQL_ROW& Row)
{
	Row = mysql_fetch_row(Result);
}


void CDBConnector::FreeResult(void)
{
	mysql_free_result(Result);
}

bool CDBConnector::GetRowCount(int& iCount)
{
	if (Result == nullptr)
	{
		return FALSE;
	}

	iCount = Result->row_count;
	return TRUE;
}