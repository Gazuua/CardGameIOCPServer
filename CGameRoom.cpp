#include "CGameRoom.h"

CGameRoom::CGameRoom()
{
	this->m_ClientList.clear();
	this->m_GameState = new CGameLogics();
	InitializeCriticalSection(&m_CS);
}

CGameRoom::~CGameRoom()
{
	this->m_ClientList.clear();
	delete this->m_GameState;
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
	int size = 0;

	EnterCriticalSection(&m_CS);
	this->m_ClientList.remove(client);
	size = this->m_ClientList.size();
	LeaveCriticalSection(&m_CS);
	return size;
}

int CGameRoom::GetRoomState()
{
	return this->m_GameState->GetGameState();
}

int CGameRoom::GetUserNumber()
{
	return this->m_ClientList.size();
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
	this->m_Name.assign(name);
}


