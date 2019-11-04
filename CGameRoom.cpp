#include "CGameRoom.h"

CGameRoom::CGameRoom()
{
	this->m_ClientList.clear();
	this->m_nRoomState = GAME_STATE_READY;
	InitializeCriticalSection(&m_CS);
}

CGameRoom::~CGameRoom()
{
	this->m_ClientList.clear();
	DeleteCriticalSection(&m_CS);
}

int CGameRoom::OnEnter(CClient* client)
{
	int size = 0;

	EnterCriticalSection(&m_CS);
	if (this->m_ClientList.size() < 4) {
		this->m_ClientList.push_back(client);
		size = this->m_ClientList.size();
		LeaveCriticalSection(&m_CS);
		return size;
	}
	else {
		LeaveCriticalSection(&m_CS);
		return -1;
	}
}

int CGameRoom::OnExit(CClient* client)
{
	int size = -1;

	EnterCriticalSection(&m_CS);
	this->m_ClientList.remove(client);
	size = this->m_ClientList.size();
	LeaveCriticalSection(&m_CS);
	return size;
}

void CGameRoom::OnStart()
{
	EnterCriticalSection(&m_CS);
	// 방 상태를 게임 시작으로 변경한다.
	this->m_nRoomState = GAME_STATE_START;

	// 기본 판돈을 설정한다.
	this->m_nMoney = 10;
	this->m_nMoneySum = 0;

	// 카드를 섞는다.
	this->InitializeCardSet();

	// 유저의 모든 상태를 서버에서 미리 설정하고
	// 유저의 게임방 입장신호를 기다린다.
	// 이후 처리는 WSARecv에서 진행된다.
	list<CClient*>::iterator iter;
	for (iter = m_ClientList.begin(); iter != m_ClientList.end(); iter++)
	{
		(*iter)->setLastBetting(GAME_BETTING_NEUTRAL);
		(*iter)->setDie(false);

		(*iter)->setBetMoney(0);
		// 카드 1장 나눠주기
		(*iter)->setCardOne(m_CardSet.back());
		m_CardSet.pop_back();
		// 1장 더 나눠주기
		(*iter)->setCardTwo(m_CardSet.back());
		m_CardSet.pop_back();

		// 족보 코드로 변환하여 저장한다.
		(*iter)->setJokboCode(CCard::GetJokboCode((*iter)->getCardOne(), (*iter)->getCardTwo()));
	}
	
	LeaveCriticalSection(&m_CS);
	// 게임 끝나고 초기화는 CClient에서 알아서 하므로 신경쓰지 않아도 된다.
}

int CGameRoom::GetRoomState()
{
	return this->m_nRoomState;
}

int CGameRoom::GetUserNumber()
{
	return this->m_ClientList.size();
}

int CGameRoom::GetMoney()
{
	return m_nMoney;
}

int CGameRoom::GetMoneySum()
{
	return m_nMoneySum;
}

list<CClient*> CGameRoom::GetUserList()
{
	return this->m_ClientList;
}

string CGameRoom::GetName()
{
	return this->m_Name;
}

void CGameRoom::SetName(string name)
{
	EnterCriticalSection(&m_CS);
	this->m_Name.assign(name); 
	LeaveCriticalSection(&m_CS);
}

void CGameRoom::SetRoomState(int state)
{
	EnterCriticalSection(&m_CS);
	this->m_nRoomState = state;
	LeaveCriticalSection(&m_CS);
}

void CGameRoom::SetMoney(int money)
{
	EnterCriticalSection(&m_CS);
	m_nMoney = money;
	LeaveCriticalSection(&m_CS);
}

void CGameRoom::AddMoneySum(int money)
{
	EnterCriticalSection(&m_CS);
	m_nMoneySum += money;
	LeaveCriticalSection(&m_CS);
}

void CGameRoom::InitializeCardSet()
{
	mt19937 engine((unsigned int)time(NULL)); // 난수 발생 엔진
	uniform_int_distribution<int> distribution(0, 19); // 난수 범위
	auto generator = bind(distribution, engine); // 난수 생성기

	// 카드를 한 장씩 세트 뒤에 추가한다.
	for (int i = 1; i <= 10; i++)
	{
		this->m_CardSet.push_back(new CCard(i, true));
		this->m_CardSet.push_back(new CCard(i, false));
	}

	// 1차로 순서대로 섞는다
	for (int i = 0; i < 20; i++)
	{
		int a = generator();
		if (a == i) a = generator();

		CCard* temp = m_CardSet[i];
		m_CardSet[i] = m_CardSet[a];
		m_CardSet[a] = temp;
	}

	// 2차로 무작위로 섞는다 (20장 카드의 3배수인 60번 섞는다)
	for (int i = 0; i < 60; i++)
	{
		int a = generator();
		int b = generator();
		if (a == b) b = generator();

		CCard* temp = m_CardSet[a];
		m_CardSet[a] = m_CardSet[b];
		m_CardSet[b] = temp;
	}
}

void CGameRoom::ReleaseCardSet()
{
	// 뒤에서부터 한장씩 카드를 해제한다.
	for (int i = 0; i < this->m_CardSet.size(); i++)
	{
		CCard* card = this->m_CardSet.back();
		delete card;
		this->m_CardSet.pop_back();
	}
	this->m_CardSet.clear();
}