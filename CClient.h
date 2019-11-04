#pragma once
#include<iostream>
#include<string>
#include<windows.h>

#include"CCard.h"

using namespace std;

#define GAME_BETTING_NEUTRAL 0
#define GAME_BETTING_CALL 1
#define GAME_BETTING_DOUBLE 2
#define GAME_BETTING_ALLIN 3
#define GAME_BETTING_DIE 4

class CClient
{
public:
	CClient(SOCKET socket);
	~CClient() {};

	void OnLoggedOn(string id, int win, int lose, int money);
	void OnEnterRoom(int room);
	void OnExitRoom();
	void OnGameSet(bool win);
	void OnEndGame();

	string getID();
	int getWin();
	int getLose();
	int getMoney();
	int getRoom();
	int getLastBetting();
	int getBetMoney();
	bool isDead();
	CCard* getCardOne();
	CCard* getCardTwo();
	int getJokboCode();
	SOCKET getSocket();

	void setID(string id);
	void setWin(int win);
	void setLose(int lose);
	void setMoney(int money);
	void setRoom(int room);
	void setLastBetting(int bet);
	void setBetMoney(int money);
	void setDie(bool die);
	void setCardOne(CCard* card);
	void setCardTwo(CCard* card);
	void setJokboCode(int code);
private:
	string m_ID;
	int m_nWin;
	int m_nLose;
	int m_nMoney;
	int m_nRoom;

	CCard* m_Card_1;
	CCard* m_Card_2;
	int m_lastBetting;
	int m_betMoney;
	bool m_bDie;
	int m_jokboCode;

	SOCKET m_Socket;
};

