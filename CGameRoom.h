#pragma once
#include<iostream>
#include<list>
#include<vector>
#include<string>
#include<random>
#include<ctime>
#include<functional>
#include<Windows.h>

#include"CClient.h"
#include"CCard.h"

using namespace std;

#define GAME_STATE_READY 0		// 게임 시작 전 대기하는 상태
#define GAME_STATE_START 1		// 게임을 시작한 상태

// 게임 흐름에 관한 모든 로직은 서버의 통제 하에 클라이언트에 전달된다.
// 클라이언트가 통제할 수 있는 조작은 베팅 뿐이다. (다이, 콜, 더블)

// 게임 규칙
// 시작하자마자 2장의 카드를 받는다.
// 기본 10원의 판돈에 대해 턴 제한 없이 누구나 자유롭게 베팅을 수행한다.
// 자신이 가진 돈 이상은 더블로 걸 수 없고, 콜만 가능하다.
// 살아남은 모든 유저가 기준 판돈과 같은 금액을 이번 턴에 베팅할 경우 게임이 끝나고 정산이 된다.
// 족보 우위에 따라 승자가 모든 돈을 다 가져간다.

// 사구, 암행어사, 땡잡이 등의 특수패는 이 게임에서 구현되지 않는다.

class CGameRoom
{
public:
	CGameRoom();
	~CGameRoom();

	// 방 정보 및 상태를 다루는 함수들
	int OnEnter(CClient* client);
	int OnExit(CClient* client);
	void OnStart();

	int GetRoomState();
	string GetName();
	int GetUserNumber();
	int GetMoney();
	int GetMoneySum();
	list<CClient*> GetUserList();
	
	void SetName(string name);
	void SetRoomState(int state);
	void SetMoney(int money);
	void AddMoneySum(int money);

	// 인게임 로직을 다루는 함수들
	void InitializeCardSet();
	void ReleaseCardSet();

private:
	// 방 정보 및 상태
	string			m_Name;
	int				m_nRoomState;
	list<CClient*>	m_ClientList;
	
	// 인게임 로직을 위한 멤버
	vector<CCard*>	m_CardSet;
	int				m_nDoubleCount;
	int				m_nMoney;
	int				m_nMoneySum;

	// 방 안의 정보를 여러 스레드에서 다룰 때의 임계영역
	CRITICAL_SECTION m_CS;

	// 인게임 로직을 다루는 함수들
};

