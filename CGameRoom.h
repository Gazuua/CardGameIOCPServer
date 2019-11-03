#pragma once
#include<iostream>
#include<list>
#include<string>
#include<Windows.h>

#include"CClient.h"
#include"CGameLogics.h"

using namespace std;

class CGameRoom
{
public:
	CGameRoom();
	~CGameRoom();

	int OnEnter(CClient* client);
	int OnExit(CClient* client);

	int GetRoomState();
	string GetName();
	int GetUserNumber();
	list<CClient*> GetUserList();
	
	void SetName(string name);

private:
	string			m_Name;
	list<CClient*>	m_ClientList;
	CGameLogics*	m_GameState;
	void (CGameRoom::*callback)();

	CRITICAL_SECTION m_CS;
};

