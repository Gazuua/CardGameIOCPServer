#pragma once
#include<iostream>
#include<string>

using namespace std;

#define GAME_STATE_READY 0		// ���� ���� �� ����ϴ� ����

class CGameLogics
{
public:
	CGameLogics();
	~CGameLogics() {};

	int GetGameState();
private:
	int m_GameState;
};

