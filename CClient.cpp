#include "CClient.h"

CClient::CClient(SOCKET socket)
{
	this->m_Socket = socket;
	this->m_ID.assign("");
	this->m_nWin = 0;
	this->m_nLose = 0;
	this->m_nMoney = 0;
	this->m_nRoom = -1;
}

void CClient::OnLoggedOn(string id, int win, int lose, int money)
{
	this->m_ID.assign(id);
	this->m_nWin = win;
	this->m_nLose = lose;
	this->m_nMoney = money;
}

void CClient::OnEnterRoom(int room)
{
	this->m_nRoom = room;
}

void CClient::OnExitRoom()
{
	this->m_nRoom = -1;
}

string CClient::getID()
{
	return this->m_ID;
}

int CClient::getWin()
{
	return this->m_nWin;
}

int CClient::getLose()
{
	return this->m_nLose;
}

int CClient::getMoney()
{
	return this->m_nMoney;
}

int CClient::getRoom()
{
	return this->m_nRoom;
}

SOCKET CClient::getSocket()
{
	return this->m_Socket;
}

void CClient::setID(string id)
{
	this->m_ID.assign(id);
}

void CClient::setWin(int win)
{
	this->m_nWin = win;
}

void CClient::setLose(int lose)
{
	this->m_nLose = lose;
}

void CClient::setMoney(int money)
{
	this->m_nMoney = money;
}

void CClient::setRoom(int room)
{
	this->m_nRoom = room;
}
