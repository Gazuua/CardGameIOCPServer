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
	// �� ���¸� ���� �������� �����Ѵ�.
	this->m_nRoomState = GAME_STATE_START;

	// �⺻ �ǵ��� �����Ѵ�.
	this->m_nMoney = 10;
	this->m_nMoneySum = 0;

	// ī�带 ���´�.
	this->InitializeCardSet();

	// ������ ��� ���¸� �������� �̸� �����ϰ�
	// ������ ���ӹ� �����ȣ�� ��ٸ���.
	// ���� ó���� WSARecv���� ����ȴ�.
	list<CClient*>::iterator iter;
	for (iter = m_ClientList.begin(); iter != m_ClientList.end(); iter++)
	{
		(*iter)->setLastBetting(GAME_BETTING_NEUTRAL);
		(*iter)->setDie(false);

		(*iter)->setBetMoney(0);
		// ī�� 1�� �����ֱ�
		(*iter)->setCardOne(m_CardSet.back());
		m_CardSet.pop_back();
		// 1�� �� �����ֱ�
		(*iter)->setCardTwo(m_CardSet.back());
		m_CardSet.pop_back();

		// ���� �ڵ�� ��ȯ�Ͽ� �����Ѵ�.
		(*iter)->setJokboCode(CCard::GetJokboCode((*iter)->getCardOne(), (*iter)->getCardTwo()));
	}
	
	LeaveCriticalSection(&m_CS);
	// ���� ������ �ʱ�ȭ�� CClient���� �˾Ƽ� �ϹǷ� �Ű澲�� �ʾƵ� �ȴ�.
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
	mt19937 engine((unsigned int)time(NULL)); // ���� �߻� ����
	uniform_int_distribution<int> distribution(0, 19); // ���� ����
	auto generator = bind(distribution, engine); // ���� ������

	// ī�带 �� �徿 ��Ʈ �ڿ� �߰��Ѵ�.
	for (int i = 1; i <= 10; i++)
	{
		this->m_CardSet.push_back(new CCard(i, true));
		this->m_CardSet.push_back(new CCard(i, false));
	}

	// 1���� ������� ���´�
	for (int i = 0; i < 20; i++)
	{
		int a = generator();
		if (a == i) a = generator();

		CCard* temp = m_CardSet[i];
		m_CardSet[i] = m_CardSet[a];
		m_CardSet[a] = temp;
	}

	// 2���� �������� ���´� (20�� ī���� 3����� 60�� ���´�)
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
	// �ڿ������� ���徿 ī�带 �����Ѵ�.
	for (int i = 0; i < this->m_CardSet.size(); i++)
	{
		CCard* card = this->m_CardSet.back();
		delete card;
		this->m_CardSet.pop_back();
	}
	this->m_CardSet.clear();
}