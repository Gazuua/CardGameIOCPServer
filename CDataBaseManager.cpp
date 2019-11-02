#include "CDataBaseManager.h"

// 싱글톤 인스턴스를 프로그램에서 호출하는 함수
CDataBaseManager* CDataBaseManager::GetInstance()
{
	// 최초 인스턴스에 메모리 할당
	if (!m_pInstance) {
		m_pInstance = new CDataBaseManager();
	}
	return m_pInstance;
}

// 메모리에서 해제
void CDataBaseManager::Release()
{
	GetInstance()->DBDisconnect();
	m_pInstance = nullptr;
}

bool CDataBaseManager::DBConnect()
{
	SQLRETURN Ret;

	// Allocate the Environment Handle (hEnv)
	if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) != SQL_SUCCESS)
	{
		return false;
	}

	// Set attributes of the Environment Handle (hEnv)
	if (SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER) != SQL_SUCCESS)
	{
		return false;
	}

	// Allocate the Connection Handle (hDbc)
	if (SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc) != SQL_SUCCESS)
	{
		return false;
	}

	// Connect to the SQL Sever with ODBC name, ID, and PW
	Ret = SQLConnect(hDbc, (SQLCHAR*)"DBSERVER", SQL_NTS, (SQLCHAR*)"CARDMASTER", SQL_NTS, (SQLCHAR*)"0000", SQL_NTS);

	if ((Ret != SQL_SUCCESS) && (Ret != SQL_SUCCESS_WITH_INFO))
	{
		return false;
	}

	puts("============= DATABASE 연결 완료!=============");

	return true;
}

void CDataBaseManager::DBDisconnect()
{
	SQLDisconnect(hDbc);					// Disconnect from the SQL Server
	SQLFreeHandle(SQL_HANDLE_DBC, hDbc);	// Free the Connection Handle
	SQLFreeHandle(SQL_HANDLE_ENV, hEnv);	// Free the Environment Handle
}

bool CDataBaseManager::LoginRequest(string id, string pw)
{
	SQLCHAR query[256];
	SQLHSTMT hStmt;
	bool ret = false;

	if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt) != SQL_SUCCESS)
		return ret;

	string stringQuery = "SELECT * FROM CLIENT WHERE USERID = '" + id + "' AND USERPW = '" + pw + "'";
	// cout << "쿼리 실행 :: " << stringQuery << endl;

	strcpy((char*)query, stringQuery.c_str());
	SQLExecDirect(hStmt, query, SQL_NTS);
	while (SQLFetch(hStmt) != SQL_NO_DATA)
		ret = true;

	SQLCloseCursor(hStmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
		
	return ret;
}
