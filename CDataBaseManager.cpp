#include "CDataBaseManager.h"

// �̱��� �ν��Ͻ��� ���α׷����� ȣ���ϴ� �Լ�
CDataBaseManager* CDataBaseManager::GetInstance()
{
	// ���� �ν��Ͻ��� �޸� �Ҵ�
	if (!m_pInstance) {
		m_pInstance = new CDataBaseManager();
	}
	return m_pInstance;
}

// �޸𸮿��� ����
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

	puts("============= DATABASE ���� �Ϸ�!=============");

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
	// cout << "���� ���� :: " << stringQuery << endl;

	// query[256]�� ���� ��Ʈ���� �����ϰ� �����Ѵ�.
	strcpy((char*)query, stringQuery.c_str());
	SQLExecDirect(hStmt, query, SQL_NTS);
	
	// SELECT���� ��������� ���� �����͸� �� ������ ���ε��Ѵ�.
	SQLCHAR userid[13];
	SQLINTEGER userwin, userlose, usermoney;
	SQLBindCol(hStmt, 1, SQL_C_CHAR, userid, 13, NULL);
	SQLBindCol(hStmt, 2, SQL_C_SLONG, &userwin, 13, NULL);
	SQLBindCol(hStmt, 3, SQL_C_SLONG, &userlose, 13, NULL);
	SQLBindCol(hStmt, 4, SQL_C_SLONG, &usermoney, 13, NULL);

	// �Ʒ� �ڵ�� SELECT�� ã�� �����͸� �� �྿ ��ȸ�ϴ� �ڵ��̴�.
	// �α��� �� ��ġ�ϴ� ���� �ݵ�� �� ���� �����Ƿ� �ٷ� �α��� ó���� �ش�.
	while (SQLFetch(hStmt) != SQL_NO_DATA)
	{
		client->OnLoggedOn((char*)userid, (int)userwin, (int)userlose, (int)usermoney);
		ret = true;
	}

	// SQL�� ������ ������ Ŀ���� �ݰ� �ڵ��� �����Ѵ�.
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
	// cout << "���� ���� :: " << stringQuery << endl;

	strcpy((char*)query, stringQuery.c_str());
	SQLExecDirect(hStmt, query, SQL_NTS);
	
	// �Ʒ� �ڵ�� SQL�� �������� ������ ���� ���� ������ ��ȯ�Ѵ�.
	// 1�� ���� ���ԵǾ��ٸ� ���������� ȸ�������� �� ���̴�.
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
	// cout << "���� ���� :: " << stringQuery << endl;

	// query[256]�� ���� ��Ʈ���� �����ϰ� �����Ѵ�.
	strcpy((char*)query, stringQuery.c_str());
	SQLExecDirect(hStmt, query, SQL_NTS);

	// SELECT���� ��������� ���� �����͸� �� ������ ���ε��Ѵ�.
	SQLCHAR userid[13];
	SQLINTEGER userwin, userlose, usermoney;
	SQLBindCol(hStmt, 1, SQL_C_CHAR, userid, 13, NULL);
	SQLBindCol(hStmt, 2, SQL_C_SLONG, &userwin, 13, NULL);
	SQLBindCol(hStmt, 3, SQL_C_SLONG, &userlose, 13, NULL);
	SQLBindCol(hStmt, 4, SQL_C_SLONG, &usermoney, 13, NULL);

	// �Ʒ� �ڵ�� SELECT�� ã�� �����͸� �� �྿ ��ȸ�ϴ� �ڵ��̴�.
	// �α��� �� ��ġ�ϴ� ���� �ݵ�� �� ���� �����Ƿ� �ٷ� �α��� ó���� �ش�.
	while (SQLFetch(hStmt) != SQL_NO_DATA)
	{
		// ��� ������������ NOT NULL�� �ɾ���� ������
		// �����͸� ã�⸸ �ϸ� NULL������ �Ű澵 �ʿ䰡 ����.
		// �׸��� ��ã���� ������ �� ������ ��������� �ʴ´�.
		client->OnLoggedOn((char*)userid, (int)userwin, (int)userlose, (int)usermoney);
		ret = true;
	}

	// SQL�� ������ ������ Ŀ���� �ݰ� �ڵ��� �����Ѵ�.
	SQLCloseCursor(hStmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

	return ret;
}
