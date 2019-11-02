#pragma once
#include<iostream>
#include<stdio.h>
#include<Windows.h>
#include<sql.h>
#include<sqlext.h>

using namespace std;

#pragma warning(disable:4996)

class CDataBaseManager
{
public:
	// Singleton Pattern ������ ���� ��� �Լ�
	static CDataBaseManager* GetInstance();

	bool DBConnect();
	void DBDisconnect();

	bool LoginRequest(string id, string pw);
	bool RegisterRequest(string id, string pw);

	void Release();

private:
	// Singleton Pattern ������ ���� ���
	static CDataBaseManager* m_pInstance;

	// SQL Environment Variables
	SQLHENV hEnv = NULL;
	SQLHDBC hDbc = NULL;
};

