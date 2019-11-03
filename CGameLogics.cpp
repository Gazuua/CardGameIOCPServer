#include "CGameLogics.h"

CGameLogics::CGameLogics()
{
	this->m_GameState = GAME_STATE_READY;
}

int CGameLogics::GetGameState()
{
	return this->m_GameState;
}

void CGameLogics::SetGameState(int state)
{
	this->m_GameState = state;
}
