#include "CGameLogics.h"

CGameLogics::CGameLogics()
{
	this->m_GameState = GAME_STATE_READY;
}

int CGameLogics::GetGameState()
{
	return this->m_GameState;
}
