#pragma once
#include<iostream>
#include<string>

using namespace std;

#define GAME_STATE_READY 0		// ���� ���� �� ����ϴ� ����
#define GAME_STATE_START 1		// ������ ������ ����

class CGameLogics
{
public:
	CGameLogics();
	~CGameLogics() {};

	int GetGameState();

	void SetGameState(int state);
private:
	int m_GameState;
};

