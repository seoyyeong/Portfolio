#include "pch.h"
#include "CDBConnectorTls.h"

void CDBConnectorTls::InitConnection(const WCHAR* DBIP, short port, const WCHAR* UserName, const WCHAR* Password, const WCHAR* DBName)
{
	size_t len;

	wcstombs_s(&len, IP, DBIP, 20);
	wcstombs_s(&len, Name, UserName, 20);
	wcstombs_s(&len, Pw, Password, 20);
	wcstombs_s(&len, DB, DBName, 20);
	Port = port;
	
	return;
}


void CDBConnectorTls::CloseConnection(void)
{
	CDBConnector* pConnector = GetConnector();

	pConnector->CloseConnection();
}

bool CDBConnectorTls::BeginTransaction(void)
{
	CDBConnector* pConnector = GetConnector();

	return pConnector->BeginTransaction();
}

bool CDBConnectorTls::EndTransaction(void)
{
	CDBConnector* pConnector = GetConnector();

	return pConnector->EndTransaction();
}

bool CDBConnectorTls::Execute(const char* pQuery)
{
	CDBConnector* pConnector = GetConnector();

	return pConnector->Execute(pQuery);
}

void CDBConnectorTls::StoreResult(void)
{
	CDBConnector* pConnector = GetConnector();

	return pConnector->StoreResult();
}

void CDBConnectorTls::Fetch(MYSQL_ROW& Row)
{
	CDBConnector* pConnector = GetConnector();

	pConnector->Fetch(Row);
}


void CDBConnectorTls::FreeResult(void)
{
	CDBConnector* pConnector = GetConnector();

	pConnector->FreeResult();
}

bool CDBConnectorTls::GetRowCount(int& iCount)
{
	CDBConnector* pConnector = GetConnector();

	return pConnector->GetRowCount(iCount);

}