#include "../ui/menu_context.hpp"
#include "../ui/connection.hpp"

MenuContext currentMenuContext()
{
	MenuContext ctx{};

	ctx.in_game = i::engine->IsInGame();

	if (!ctx.in_game) {
		return ctx;
	}

	C_TFPlayer *const local{ ec->getLocal() };

	if (!local || local->deadflag()) {
		return ctx;
	}

	ctx.player_class = local->m_iClass();

	C_TFWeaponBase *const weapon{ ec->getWeapon() };

	if (!weapon) {
		return ctx;
	}

	const int weapon_id{ weapon->GetWeaponID() };

	if (weapon_id == TF_WEAPON_MEDIGUN) {
		ctx.weapon = HeldWeapon::MEDIGUN;
	}

	else if (tf_utils::isWeaponMelee(weapon)) {
		ctx.weapon = HeldWeapon::MELEE;
	}

	else if (tf_utils::isWeaponHitscan(weapon)) {
		ctx.weapon = HeldWeapon::HITSCAN;
	}

	else if (tf_utils::isWeaponProjectile(weapon)) {
		ctx.weapon = HeldWeapon::PROJECTILE;
	}

	else {
		ctx.weapon = HeldWeapon::OTHER;
	}

	return ctx;
}

ConnectionSample currentConnection()
{
	ConnectionSample sample{};

	INetChannelInfo *const net{ i::engine->GetNetChannelInfo() };

	if (!net || !i::engine->IsInGame()) {
		return sample;
	}

	sample.connected = true;
	sample.loopback = net->IsLoopback();
	sample.tickrate = i::global_vars->interval_per_tick > 0.0f ? 1.0f / i::global_vars->interval_per_tick : 0.0f;
	sample.map = i::client_state->m_szLevelBaseName;

	if (sample.loopback) {
		return sample;
	}

	if (const char *const address{ net->GetAddress() }) {
		sample.address = address;
	}

	if (const char *const name{ net->GetName() }) {
		sample.server = name;
	}

	sample.time_connected = net->GetTimeConnected();
	sample.latency_out = net->GetAvgLatency(FLOW_OUTGOING);
	sample.latency_in = net->GetAvgLatency(FLOW_INCOMING);
	sample.loss_out = net->GetAvgLoss(FLOW_OUTGOING);
	sample.loss_in = net->GetAvgLoss(FLOW_INCOMING);
	sample.choke_out = net->GetAvgChoke(FLOW_OUTGOING);
	sample.choke_in = net->GetAvgChoke(FLOW_INCOMING);
	sample.data_out = net->GetAvgData(FLOW_OUTGOING);
	sample.data_in = net->GetAvgData(FLOW_INCOMING);

	//the scoreboard ping, falls back to the net channel's latency before the resource exists
	C_TFPlayerResource *const resource{ ec->getPlayerResource() };

	if (resource) {
		sample.ping = resource->GetPing(i::engine->GetLocalPlayer());
	}

	else {
		sample.ping = static_cast<int>((sample.latency_out + sample.latency_in) * 1000.0f);
	}

	return sample;
}
