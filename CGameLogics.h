#pragma once
#include<iostream>
#include<string>

using namespace std;

#define GAME_STATE_READY 0		// 게임 시작 전 대기하는 상태
#define GAME_STATE_START 1		// 게임을 시작한 상태

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

