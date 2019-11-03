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

bool CDataBaseManager::LoginRequest(string id, string pw, CClient* client)
{
	SQLCHAR query[256];
	SQLHSTMT hStmt;
	bool ret = false;

	if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt) != SQL_SUCCESS)
		return ret;

	string stringQuery = "SELECT USERID, USERWIN, USERLOSE, USERMONEY FROM CLIENT WHERE USERID = '" 
		+ id + "' AND USERPW = '" + pw + "'";
	// cout << "쿼리 실행 :: " << stringQuery << endl;

	// query[256]에 쿼리 스트링을 복사하고 실행한다.
	strcpy((char*)query, stringQuery.c_str());
	SQLExecDirect(hStmt, query, SQL_NTS);
	
	// SELECT문의 결과값으로 나올 데이터를 각 변수에 바인딩한다.
	SQLCHAR userid[13];
	SQLINTEGER userwin, userlose, usermoney;
	SQLBindCol(hStmt, 1, SQL_C_CHAR, userid, 13, NULL);
	SQLBindCol(hStmt, 2, SQL_C_SLONG, &userwin, 13, NULL);
	SQLBindCol(hStmt, 3, SQL_C_SLONG, &userlose, 13, NULL);
	SQLBindCol(hStmt, 4, SQL_C_SLONG, &usermoney, 13, NULL);

	// 아래 코드는 SELECT로 찾은 데이터를 한 행씩 순회하는 코드이다.
	// 로그인 시 일치하는 행은 반드시 한 개만 나오므로 바로 로그인 처리해 준다.
	while (SQLFetch(hStmt) != SQL_NO_DATA)
	{
		client->OnLoggedOn((char*)userid, (int)userwin, (int)userlose, (int)usermoney);
		ret = true;
	}

	// SQL문 실행이 끝나면 커서를 닫고 핸들을 해제한다.
	SQLCloseCursor(hStmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
		
	return ret;
}

bool CDataBaseManager::RegisterRequest(string id, string pw)
{
	SQLCHAR query[256];
	SQLHSTMT hStmt;
	SQLINTEGER row = 0;
	bool ret = false;

	if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt) != SQL_SUCCESS)
		return ret;

	string stringQuery = "INSERT INTO CLIENT VALUES('" + id + "', '" + pw + "', 0, 0, 10000)";
	// cout << "쿼리 실행 :: " << stringQuery << endl;

	strcpy((char*)query, stringQuery.c_str());
	SQLExecDirect(hStmt, query, SQL_NTS);
	
	// 아래 코드는 SQL문 실행으로 영향을 받은 행의 갯수를 반환한다.
	// 1개 행이 삽입되었다면 정상적으로 회원가입이 된 것이다.
	SQLRowCount(hStmt, &row);
	if (row == 1) ret = true;

	SQLCloseCursor(hStmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

	return ret;
}

bool CDataBaseManager::UserInfoRequest(string id, CClient* client)
{
	SQLCHAR query[256];
	SQLHSTMT hStmt;
	bool ret = false;

	if (SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt) != SQL_SUCCESS)
		return ret;

	string stringQuery = "SELECT USERID, USERWIN, USERLOSE, USERMONEY FROM CLIENT WHERE USERID = '" + id + "'";
	// cout << "쿼리 실행 :: " << stringQuery << endl;

	// query[256]에 쿼리 스트링을 복사하고 실행한다.
	strcpy((char*)query, stringQuery.c_str());
	SQLExecDirect(hStmt, query, SQL_NTS);

	// SELECT문의 결과값으로 나올 데이터를 각 변수에 바인딩한다.
	SQLCHAR userid[13];
	SQLINTEGER userwin, userlose, usermoney;
	SQLBindCol(hStmt, 1, SQL_C_CHAR, userid, 13, NULL);
	SQLBindCol(hStmt, 2, SQL_C_SLONG, &userwin, 13, NULL);
	SQLBindCol(hStmt, 3, SQL_C_SLONG, &userlose, 13, NULL);
	SQLBindCol(hStmt, 4, SQL_C_SLONG, &usermoney, 13, NULL);

	// 아래 코드는 SELECT로 찾은 데이터를 한 행씩 순회하는 코드이다.
	// 로그인 시 일치하는 행은 반드시 한 개만 나오므로 바로 로그인 처리해 준다.
	while (SQLFetch(hStmt) != SQL_NO_DATA)
	{
		// 디비에 제약조건으로 NOT NULL을 걸어놨기 때문에
		// 데이터를 찾기만 하면 NULL값인지 신경쓸 필요가 없다.
		// 그리고 못찾으면 어차피 이 문장은 실행되지도 않는다.
		client->OnLoggedOn((char*)userid, (int)userwin, (int)userlose, (int)usermoney);
		ret = true;
	}

	// SQL문 실행이 끝나면 커서를 닫고 핸들을 해제한다.
	SQLCloseCursor(hStmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

	return ret;
}
