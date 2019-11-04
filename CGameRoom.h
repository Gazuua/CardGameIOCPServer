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

#define GAME_STATE_READY 0		// ���� ���� �� ����ϴ� ����
#define GAME_STATE_START 1		// ������ ������ ����

// ���� �帧�� ���� ��� ������ ������ ���� �Ͽ� Ŭ���̾�Ʈ�� ���޵ȴ�.
// Ŭ���̾�Ʈ�� ������ �� �ִ� ������ ���� ���̴�. (����, ��, ����)

// ���� ��Ģ
// �������ڸ��� 2���� ī�带 �޴´�.
// �⺻ 10���� �ǵ��� ���� �� ���� ���� ������ �����Ӱ� ������ �����Ѵ�.
// �ڽ��� ���� �� �̻��� ����� �� �� ����, �ݸ� �����ϴ�.
// ��Ƴ��� ��� ������ ���� �ǵ��� ���� �ݾ��� �̹� �Ͽ� ������ ��� ������ ������ ������ �ȴ�.
// ���� ������ ���� ���ڰ� ��� ���� �� ��������.

// �籸, ������, ������ ���� Ư���д� �� ���ӿ��� �������� �ʴ´�.

class CGameRoom
{
public:
	CGameRoom();
	~CGameRoom();

	// �� ���� �� ���¸� �ٷ�� �Լ���
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

	// �ΰ��� ������ �ٷ�� �Լ���
	void InitializeCardSet();
	void ReleaseCardSet();

private:
	// �� ���� �� ����
	string			m_Name;
	int				m_nRoomState;
	list<CClient*>	m_ClientList;
	
	// �ΰ��� ������ ���� ���
	vector<CCard*>	m_CardSet;
	int				m_nDoubleCount;
	int				m_nMoney;
	int				m_nMoneySum;

	// �� ���� ������ ���� �����忡�� �ٷ� ���� �Ӱ迵��
	CRITICAL_SECTION m_CS;

	// �ΰ��� ������ �ٷ�� �Լ���
};

