#include "statistics.hpp"

StatsLocal currentStatsLocal()
{
	StatsLocal local{};

	local.in_game = i::engine->IsInGame();

	if (!local.in_game) {
		return local;
	}

	local.entindex = i::engine->GetLocalPlayer();
	local.map = i::client_state->m_szLevelBaseName;

	player_info_t info{};

	if (i::engine->GetPlayerInfo(local.entindex, &info)) {
		local.userid = info.userID;
	}

	C_TFPlayer *const player{ ec->getLocal() };

	if (!player) {
		return local;
	}

	local.team = player->m_iTeamNum();
	local.player_class = player->m_iClass();
	local.alive = !player->deadflag();

	if (C_TFWeaponBase *const weapon{ player->m_hActiveWeapon()->cast<C_TFWeaponBase>() }) {
		local.melee = weapon->GetSlot() == 2;
	}

	return local;
}
