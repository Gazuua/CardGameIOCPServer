#pragma once
#include<iostream>
#include<string>
#include<windows.h>

using namespace std;

class CClient
{
public:
	CClient(SOCKET socket);
	~CClient() {};

	void OnLoggedOn(string id, int win, int lose, int money);
	void OnEnterRoom(int room);
	void OnExitRoom();

	string getID();
	int getWin();
	int getLose();
	int getMoney();
	int getRoom();

	void setID(string id);
	void setWin(int win);
	void setLose(int lose);
	void setMoney(int money);
	void setRoom(int room);
private:
	string m_ID;
	int m_nWin;
	int m_nLose;
	int m_nMoney;
	int m_nRoom;

	SOCKET m_Socket;
};

