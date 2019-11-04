#pragma once
#include<iostream>
#include<stdio.h>
#include<Windows.h>
#include<sql.h>
#include<sqlext.h>

#include"CClient.h"

using namespace std;

#pragma warning(disable:4996)

class CDataBaseManager
{
public:
	// Singleton Pattern 구현을 위한 멤버 함수
	static CDataBaseManager* GetInstance();

	bool DBConnect();
	void DBDisconnect();

	bool LoginRequest(string id, string pw, CClient* client);
	bool RegisterRequest(string id, string pw);
	bool UserInfoRequest(string id, CClient* client);
	bool UserWinRequest(CClient* client, int money);
	bool UserLoseRequest(CClient* client, int money);

	void Release();

private:
	// Singleton Pattern 구현을 위한 멤버
	static CDataBaseManager* m_pInstance;

	// SQL Environment Variables
	SQLHENV hEnv = NULL;
	SQLHDBC hDbc = NULL;
};

