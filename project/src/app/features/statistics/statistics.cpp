#include "statistics.hpp"

#include "../notifs/notifs.hpp"

#include <chrono>
#include <filesystem>

//--------------------------------------------------------------------------------------------------

static const StatInfo stat_infos[]
{
	{ "kills", "kills", StatGroup::COMBAT, StatKind::COUNT },
	{ "deaths", "deaths", StatGroup::COMBAT, StatKind::COUNT },
	{ "assists", "assists", StatGroup::COMBAT, StatKind::COUNT },
	{ "headshots", "headshots", StatGroup::COMBAT, StatKind::COUNT },
	{ "backstabs", "backstabs", StatGroup::COMBAT, StatKind::COUNT },
	{ "melee_kills", "melee kills", StatGroup::COMBAT, StatKind::COUNT },
	{ "crit_kills", "crit kills", StatGroup::COMBAT, StatKind::COUNT },
	{ "minicrit_kills", "mini-crit kills", StatGroup::COMBAT, StatKind::COUNT },
	{ "dominations", "dominations", StatGroup::COMBAT, StatKind::COUNT },
	{ "assisted_dominations", "assisted dominations", StatGroup::COMBAT, StatKind::COUNT },
	{ "revenges", "revenges", StatGroup::COMBAT, StatKind::COUNT },
	{ "assisted_revenges", "assisted revenges", StatGroup::COMBAT, StatKind::COUNT },
	{ "times_dominated", "times dominated", StatGroup::COMBAT, StatKind::COUNT },
	{ "times_avenged_on", "times avenged on", StatGroup::COMBAT, StatKind::COUNT },
	{ "first_bloods", "first bloods", StatGroup::COMBAT, StatKind::COUNT },
	{ "gibs", "gibs", StatGroup::COMBAT, StatKind::COUNT },
	{ "taunt_kills", "taunt kills", StatGroup::COMBAT, StatKind::COUNT },
	{ "telefrags", "telefrags", StatGroup::COMBAT, StatKind::COUNT },
	{ "environmental_deaths", "environmental deaths", StatGroup::COMBAT, StatKind::COUNT },
	{ "suicides", "suicides", StatGroup::COMBAT, StatKind::COUNT },
	{ "feign_deaths", "feign deaths", StatGroup::COMBAT, StatKind::COUNT },

	{ "damage_dealt", "damage dealt", StatGroup::DAMAGE, StatKind::COUNT },
	{ "damage_taken", "damage taken", StatGroup::DAMAGE, StatKind::COUNT },
	{ "crit_damage", "crit damage", StatGroup::DAMAGE, StatKind::COUNT },
	{ "minicrit_damage", "mini-crit damage", StatGroup::DAMAGE, StatKind::COUNT },
	{ "biggest_hit", "biggest hit", StatGroup::DAMAGE, StatKind::RECORD },
	{ "self_damage", "self damage", StatGroup::DAMAGE, StatKind::COUNT },

	{ "healing", "healing", StatGroup::CLASS, StatKind::COUNT },
	{ "ubers", "ubercharges", StatGroup::CLASS, StatKind::COUNT },
	{ "medics_killed", "medics killed", StatGroup::CLASS, StatKind::COUNT },
	{ "charged_medics_killed", "charged medics killed", StatGroup::CLASS, StatKind::COUNT },
	{ "sentries_destroyed", "sentries destroyed", StatGroup::CLASS, StatKind::COUNT },
	{ "dispensers_destroyed", "dispensers destroyed", StatGroup::CLASS, StatKind::COUNT },
	{ "teleporters_destroyed", "teleporters destroyed", StatGroup::CLASS, StatKind::COUNT },
	{ "own_buildings_lost", "own buildings lost", StatGroup::CLASS, StatKind::COUNT },
	{ "buildings_built", "buildings built", StatGroup::CLASS, StatKind::COUNT },
	{ "teammates_teleported", "teammates teleported", StatGroup::CLASS, StatKind::COUNT },
	{ "reflects", "projectiles reflected", StatGroup::CLASS, StatKind::COUNT },
	{ "extinguishes", "teammates extinguished", StatGroup::CLASS, StatKind::COUNT },
	{ "ignites", "players ignited", StatGroup::CLASS, StatKind::COUNT },
	{ "jarates", "jarate / milk hits", StatGroup::CLASS, StatKind::COUNT },
	{ "stuns", "stuns", StatGroup::CLASS, StatKind::COUNT },
	{ "sappers", "sappers placed", StatGroup::CLASS, StatKind::COUNT },

	{ "captures", "points captured", StatGroup::OBJECTIVES, StatKind::COUNT },
	{ "captures_blocked", "captures blocked", StatGroup::OBJECTIVES, StatKind::COUNT },
	{ "flag_pickups", "flags picked up", StatGroup::OBJECTIVES, StatKind::COUNT },
	{ "flag_captures", "flags captured", StatGroup::OBJECTIVES, StatKind::COUNT },
	{ "flag_defends", "flags defended", StatGroup::OBJECTIVES, StatKind::COUNT },
	{ "rounds_won", "rounds won", StatGroup::OBJECTIVES, StatKind::COUNT },
	{ "rounds_lost", "rounds lost", StatGroup::OBJECTIVES, StatKind::COUNT },

	{ "best_killstreak", "best killstreak", StatGroup::RECORDS, StatKind::RECORD },
	{ "longest_life", "longest life", StatGroup::RECORDS, StatKind::RECORD },
	{ "most_round_kills", "most kills in a round", StatGroup::RECORDS, StatKind::RECORD },
	{ "most_life_damage", "most damage in a life", StatGroup::RECORDS, StatKind::RECORD },

	{ "time_in_game", "time in game", StatGroup::TIME, StatKind::TIME },
	{ "sessions", "sessions", StatGroup::TIME, StatKind::COUNT },
	{ "time_loaded", "time loaded", StatGroup::TIME, StatKind::TIME },
};

static_assert(std::size(stat_infos) == static_cast<size_t>(Stat::COUNT), "every stat needs an entry in stat_infos");

const StatInfo &statInfo(const Stat stat)
{
	return stat_infos[static_cast<size_t>(stat)];
}

//--------------------------------------------------------------------------------------------------
// values from tf_shareddefs.h

namespace
{
	constexpr int CUSTOM_HEADSHOT{ 1 };
	constexpr int CUSTOM_BACKSTAB{ 2 };
	constexpr int CUSTOM_SUICIDE{ 6 };
	constexpr int CUSTOM_PENETRATE_HEADSHOT{ 14 };
	constexpr int CUSTOM_TELEFRAG{ 16 };
	constexpr int CUSTOM_HEADSHOT_DECAPITATION{ 51 };

	constexpr int DEATH_DOMINATION{ 0x0001 };
	constexpr int DEATH_ASSISTER_DOMINATION{ 0x0002 };
	constexpr int DEATH_REVENGE{ 0x0004 };
	constexpr int DEATH_ASSISTER_REVENGE{ 0x0008 };
	constexpr int DEATH_FIRST_BLOOD{ 0x0010 };
	constexpr int DEATH_FEIGN_DEATH{ 0x0020 };
	constexpr int DEATH_GIBBED{ 0x0080 };

	constexpr int CRIT_TYPE_MINI{ 1 };
	constexpr int CRIT_TYPE_FULL{ 2 };

	constexpr int OBJ_DISPENSER{ 0 };
	constexpr int OBJ_TELEPORTER{ 1 };
	constexpr int OBJ_SENTRYGUN{ 2 };

	//the weapon types every melee weapon reports, all-class melees take their class's type.
	//projectiles thrown by melee weapons (sandman ball, wrap assassin ornament, cleaver) have their own types
	bool isMeleeWeaponID(const int id)
	{
		switch (id)
		{
			case TF_WEAPON_BAT:
			case TF_WEAPON_BAT_WOOD:
			case TF_WEAPON_BAT_FISH:
			case TF_WEAPON_BAT_GIFTWRAP:
			case TF_WEAPON_BOTTLE:
			case TF_WEAPON_STICKBOMB:
			case TF_WEAPON_SWORD:
			case TF_WEAPON_FIREAXE:
			case TF_WEAPON_BREAKABLE_SIGN:
			case TF_WEAPON_SLAP:
			case TF_WEAPON_CLUB:
			case TF_WEAPON_CROWBAR:
			case TF_WEAPON_KNIFE:
			case TF_WEAPON_FISTS:
			case TF_WEAPON_SHOVEL:
			case TF_WEAPON_WRENCH:
			case TF_WEAPON_BONESAW:
			case TF_WEAPON_HARVESTER_SAW: {
				return true;
			}

			default: {
				return false;
			}
		}
	}

	constexpr int FLAG_PICKUP{ 1 };
	constexpr int FLAG_CAPTURE{ 2 };
	constexpr int FLAG_DEFEND{ 3 };
}

//--------------------------------------------------------------------------------------------------

//class and map scopes skip the ones that are about the dll rather than a game
static bool perScope(const Stat stat)
{
	return stat != Stat::SESSIONS && stat != Stat::TIME_LOADED;
}

void Statistics::add(const Stat stat, const double value, const StatsLocal &local)
{
	const size_t i{ static_cast<size_t>(stat) };

	m_all_time[i] += value;
	m_session[i] += value;

	if (perScope(stat))
	{
		if (local.player_class > 0 && local.player_class < stats_class_count) {
			m_per_class[local.player_class][i] += value;
		}

		if (!local.map.empty()) {
			m_per_map[local.map][i] += value;
		}
	}

	m_dirty = true;
}

void Statistics::record(const Stat stat, const double value, const StatsLocal &local)
{
	const size_t i{ static_cast<size_t>(stat) };

	auto raise = [&](StatBlock &block) {
		block[i] = std::max(block[i], value);
	};

	raise(m_all_time);
	raise(m_session);

	if (local.player_class > 0 && local.player_class < stats_class_count) {
		raise(m_per_class[local.player_class]);
	}

	if (!local.map.empty()) {
		raise(m_per_map[local.map]);
	}

	m_dirty = true;
}

void Statistics::startLife()
{
	m_life_active = true;
	m_life_time = 0.0;
	m_life_kills = 0;
	m_life_damage = 0.0;
}

void Statistics::endLife(const StatsLocal &local)
{
	if (m_life_active) {
		record(Stat::LONGEST_LIFE, m_life_time, local);
	}

	m_life_active = false;
	m_life_kills = 0;
	m_life_damage = 0.0;
}

void Statistics::endRound(const StatsLocal &local)
{
	record(Stat::MOST_ROUND_KILLS, m_round_kills, local);

	m_round_kills = 0;
}

//--------------------------------------------------------------------------------------------------

void Statistics::handleEvent(IGameEvent *const event, const StatsLocal &local)
{
	if (!event || !m_active || local.userid <= 0) {
		return;
	}

	const std::scoped_lock lock{ m_mutex };

	const std::string_view name{ event->GetName() };

	const int me{ local.userid };
	const int my_ent{ local.entindex };

	if (name == "player_death")
	{
		const int victim{ event->GetInt("userid") };
		const int attacker{ event->GetInt("attacker") };
		const int assister{ event->GetInt("assister", -1) };
		const int custom{ event->GetInt("customkill") };
		const int flags{ event->GetInt("death_flags") };
		const int crit{ event->GetInt("crit_type") };
		const std::string weapon{ event->GetString("weapon") };

		//a dead ringer "death" isn't one, for either side
		const bool feign{ (flags & DEATH_FEIGN_DEATH) != 0 };

		if (victim == me)
		{
			if (feign) {
				add(Stat::FEIGN_DEATHS, 1, local);
				return;
			}

			add(Stat::DEATHS, 1, local);

			if (attacker == me || (attacker == 0 && custom == CUSTOM_SUICIDE)) {
				add(Stat::SUICIDES, 1, local);
			}

			else if (attacker == 0) {
				add(Stat::ENVIRONMENTAL_DEATHS, 1, local);
			}

			else
			{
				if (flags & DEATH_DOMINATION) {
					add(Stat::TIMES_DOMINATED, 1, local);
				}

				if (flags & DEATH_REVENGE) {
					add(Stat::TIMES_AVENGED_ON, 1, local);
				}
			}

			endLife(local);

			return;
		}

		if (feign) {
			return;
		}

		if (attacker == me)
		{
			add(Stat::KILLS, 1, local);

			if (!weapon.empty()) {
				m_per_weapon[weapon]++;
			}

			if (custom == CUSTOM_HEADSHOT || custom == CUSTOM_PENETRATE_HEADSHOT || custom == CUSTOM_HEADSHOT_DECAPITATION) {
				add(Stat::HEADSHOTS, 1, local);
			}

			if (custom == CUSTOM_BACKSTAB) {
				add(Stat::BACKSTABS, 1, local);
			}

			if (custom == CUSTOM_TELEFRAG) {
				add(Stat::TELEFRAGS, 1, local);
			}

			//taunt kills all use a taunt_ kill icon
			if (weapon.starts_with("taunt_")) {
				add(Stat::TAUNT_KILLS, 1, local);
			}

			if (isMeleeWeaponID(event->GetInt("weaponid", TF_WEAPON_NONE))) {
				add(Stat::MELEE_KILLS, 1, local);
			}

			if (crit == CRIT_TYPE_FULL) {
				add(Stat::CRIT_KILLS, 1, local);
			}

			else if (crit == CRIT_TYPE_MINI) {
				add(Stat::MINICRIT_KILLS, 1, local);
			}

			if (flags & DEATH_DOMINATION) {
				add(Stat::DOMINATIONS, 1, local);
			}

			if (flags & DEATH_REVENGE) {
				add(Stat::REVENGES, 1, local);
			}

			if (flags & DEATH_FIRST_BLOOD) {
				add(Stat::FIRST_BLOODS, 1, local);
			}

			if (flags & DEATH_GIBBED) {
				add(Stat::GIBS, 1, local);
			}

			m_life_kills++;
			m_round_kills++;

			record(Stat::BEST_KILLSTREAK, m_life_kills, local);
		}

		else if (assister == me)
		{
			add(Stat::ASSISTS, 1, local);

			if (flags & DEATH_ASSISTER_DOMINATION) {
				add(Stat::ASSISTED_DOMINATIONS, 1, local);
			}

			if (flags & DEATH_ASSISTER_REVENGE) {
				add(Stat::ASSISTED_REVENGES, 1, local);
			}
		}
	}

	else if (name == "player_hurt")
	{
		const int victim{ event->GetInt("userid") };
		const int attacker{ event->GetInt("attacker") };
		const double amount{ static_cast<double>(std::max(event->GetInt("damageamount"), 0)) };

		if (attacker == me && victim != me)
		{
			add(Stat::DAMAGE_DEALT, amount, local);

			if (event->GetBool("crit")) {
				add(Stat::CRIT_DAMAGE, amount, local);
			}

			else if (event->GetBool("minicrit")) {
				add(Stat::MINICRIT_DAMAGE, amount, local);
			}

			record(Stat::BIGGEST_HIT, amount, local);

			m_life_damage += amount;
			record(Stat::MOST_LIFE_DAMAGE, m_life_damage, local);
		}

		else if (victim == me) {
			add(attacker == me ? Stat::SELF_DAMAGE : Stat::DAMAGE_TAKEN, amount, local);
		}
	}

	else if (name == "player_healed")
	{
		if (event->GetInt("healer") == me && event->GetInt("patient") != me) {
			add(Stat::HEALING, std::max(event->GetInt("amount"), 0), local);
		}
	}

	else if (name == "player_chargedeployed")
	{
		if (event->GetInt("userid") == me) {
			add(Stat::UBERS, 1, local);
		}
	}

	else if (name == "medic_death")
	{
		if (event->GetInt("attacker") == me && event->GetInt("userid") != me)
		{
			add(Stat::MEDICS_KILLED, 1, local);

			if (event->GetBool("charged")) {
				add(Stat::CHARGED_MEDICS_KILLED, 1, local);
			}
		}
	}

	else if (name == "object_destroyed")
	{
		const int owner{ event->GetInt("userid") };
		const int attacker{ event->GetInt("attacker") };
		const int type{ event->GetInt("objecttype") };

		if (attacker == me && owner != me)
		{
			if (type == OBJ_SENTRYGUN) {
				add(Stat::SENTRIES_DESTROYED, 1, local);
			}

			else if (type == OBJ_DISPENSER) {
				add(Stat::DISPENSERS_DESTROYED, 1, local);
			}

			else if (type == OBJ_TELEPORTER) {
				add(Stat::TELEPORTERS_DESTROYED, 1, local);
			}
		}

		else if (owner == me && attacker != me && type <= OBJ_SENTRYGUN) {
			add(Stat::OWN_BUILDINGS_LOST, 1, local);
		}
	}

	else if (name == "player_builtobject")
	{
		//sappers count as "built" too, they're tracked through player_sapped_object instead
		if (event->GetInt("userid") == me && event->GetInt("object") <= OBJ_SENTRYGUN) {
			add(Stat::BUILDINGS_BUILT, 1, local);
		}
	}

	else if (name == "player_sapped_object")
	{
		if (event->GetInt("userid") == me) {
			add(Stat::SAPPERS, 1, local);
		}
	}

	else if (name == "player_teleported")
	{
		if (event->GetInt("builderid") == me && event->GetInt("userid") != me) {
			add(Stat::TEAMMATES_TELEPORTED, 1, local);
		}
	}

	else if (name == "object_deflected")
	{
		if (event->GetInt("userid") == me) {
			add(Stat::REFLECTS, 1, local);
		}
	}

	//these few use entity indexes rather than user ids
	else if (name == "player_extinguished")
	{
		if (event->GetInt("healer") == my_ent && event->GetInt("victim") != my_ent) {
			add(Stat::EXTINGUISHES, 1, local);
		}
	}

	else if (name == "player_ignited")
	{
		if (event->GetInt("pyro_entindex") == my_ent && event->GetInt("victim_entindex") != my_ent) {
			add(Stat::IGNITES, 1, local);
		}
	}

	else if (name == "player_jarated")
	{
		if (event->GetInt("thrower_entindex") == my_ent && event->GetInt("victim_entindex") != my_ent) {
			add(Stat::JARATES, 1, local);
		}
	}

	else if (name == "player_stunned")
	{
		if (event->GetInt("stunner") == me && event->GetInt("victim") != me) {
			add(Stat::STUNS, 1, local);
		}
	}

	else if (name == "teamplay_point_captured")
	{
		//every capper's entity index, one character each
		for (const char c : std::string_view{ event->GetString("cappers") })
		{
			if (static_cast<unsigned char>(c) == my_ent) {
				add(Stat::CAPTURES, 1, local);
				break;
			}
		}
	}

	else if (name == "teamplay_capture_blocked")
	{
		if (event->GetInt("blocker") == my_ent) {
			add(Stat::CAPTURES_BLOCKED, 1, local);
		}
	}

	else if (name == "teamplay_flag_event")
	{
		if (event->GetInt("player") == my_ent)
		{
			const int type{ event->GetInt("eventtype") };

			if (type == FLAG_PICKUP) {
				add(Stat::FLAG_PICKUPS, 1, local);
			}

			else if (type == FLAG_CAPTURE) {
				add(Stat::FLAG_CAPTURES, 1, local);
			}

			else if (type == FLAG_DEFEND) {
				add(Stat::FLAG_DEFENDS, 1, local);
			}
		}
	}

	else if (name == "teamplay_round_win")
	{
		//only players on red or blu won or lost anything
		if (local.team == 2 || local.team == 3) {
			add(event->GetInt("team") == local.team ? Stat::ROUNDS_WON : Stat::ROUNDS_LOST, 1, local);
		}

		endRound(local);
	}

	else if (name == "teamplay_round_start")
	{
		m_round_kills = 0;
	}

	else if (name == "player_spawn")
	{
		if (event->GetInt("userid") == me) {
			startLife();
		}
	}
}

void Statistics::tick(const double dt, const StatsLocal &local)
{
	if (!m_active || dt <= 0.0) {
		return;
	}

	const std::scoped_lock lock{ m_mutex };

	add(Stat::TIME_LOADED, dt, local);

	if (local.in_game) {
		add(Stat::TIME_IN_GAME, dt, local);
	}

	//picks up a life that was already going when the dll loaded
	if (local.in_game && local.alive && !m_life_active) {
		startLife();
	}

	if (m_life_active)
	{
		if (local.in_game && local.alive) {
			m_life_time += dt;
		}

		//left the server or died without an event reaching us
		else if (!local.in_game) {
			endLife(local);
		}
	}

	m_save_timer += dt;

	if (m_save_timer >= 30.0)
	{
		m_save_timer = 0.0;

		if (m_dirty) {
			saveLocked();
		}
	}
}

//--------------------------------------------------------------------------------------------------

static std::string statsPath()
{
	return utils::getFilePath() + "stats.json";
}

static nlohmann::json blockToJson(const StatBlock &block)
{
	nlohmann::json j = nlohmann::json::object();

	for (size_t i{}; i < block.size(); i++)
	{
		//zeros are left out to keep the file small
		if (block[i] != 0.0) {
			j[stat_infos[i].id] = block[i];
		}
	}

	return j;
}

static StatBlock blockFromJson(const nlohmann::json &j)
{
	StatBlock block{};

	for (size_t i{}; i < block.size(); i++)
	{
		if (j.is_object() && j.contains(stat_infos[i].id) && j[stat_infos[i].id].is_number()) {
			block[i] = j[stat_infos[i].id].get<double>();
		}
	}

	return block;
}

static const char *const class_keys[stats_class_count]{ "", "scout", "sniper", "soldier", "demoman", "medic", "heavy", "pyro", "spy", "engineer" };

void Statistics::saveLocked() const
{
	nlohmann::json j{};

	j["version"] = 1;
	j["tracking"] = m_active.load();
	j["all_time"] = blockToJson(m_all_time);

	for (int c{ 1 }; c < stats_class_count; c++) {
		j["per_class"][class_keys[c]] = blockToJson(m_per_class[c]);
	}

	j["per_map"] = nlohmann::json::object();

	for (const auto &[map, block] : m_per_map) {
		j["per_map"][map] = blockToJson(block);
	}

	j["per_weapon"] = m_per_weapon;

	//written next to the real file then swapped in, so a crash mid-write can't eat the stats
	const std::string path{ statsPath() };
	const std::string tmp{ path + ".tmp" };

	{
		std::ofstream file{ tmp };

		if (!file.is_open()) {
			return;
		}

		file << std::setw(4) << j;
	}

	std::error_code ec{};
	std::filesystem::rename(tmp, path, ec);
}

void Statistics::save() const
{
	const std::scoped_lock lock{ m_mutex };

	saveLocked();
}

void Statistics::load()
{
	const std::scoped_lock lock{ m_mutex };

	std::ifstream file{ statsPath() };

	if (!file.is_open()) {
		return;
	}

	try
	{
		nlohmann::json j{};

		file >> j;

		m_active = j.value("tracking", true);
		m_all_time = blockFromJson(j.value("all_time", nlohmann::json::object()));

		if (j.contains("per_class"))
		{
			for (int c{ 1 }; c < stats_class_count; c++) {
				m_per_class[c] = blockFromJson(j["per_class"].value(class_keys[c], nlohmann::json::object()));
			}
		}

		if (j.contains("per_map") && j["per_map"].is_object())
		{
			for (const auto &[map, block] : j["per_map"].items()) {
				m_per_map[map] = blockFromJson(block);
			}
		}

		if (j.contains("per_weapon") && j["per_weapon"].is_object())
		{
			for (const auto &[weapon, kills] : j["per_weapon"].items())
			{
				if (kills.is_number()) {
					m_per_weapon[weapon] = kills.get<double>();
				}
			}
		}
	}

	catch (const std::exception &e)
	{
		file.close();

		//kept to the side instead of being overwritten by the next save
		std::error_code ec{};
		std::filesystem::rename(statsPath(), statsPath() + ".bak", ec);

		notifs->warning(std::format("stats.json couldn't be read, moved it to stats.json.bak ({})", e.what()));

		m_all_time = {};
		m_per_class = {};
		m_per_map.clear();
		m_per_weapon.clear();
	}
}

bool Statistics::onLoad()
{
	load();

	if (m_active)
	{
		const std::scoped_lock lock{ m_mutex };

		add(Stat::SESSIONS, 1, StatsLocal{});
	}

	return true;
}

bool Statistics::onUnload()
{
	save();

	return true;
}

bool Statistics::onGameEvent(IGameEvent *const event)
{
	handleEvent(event, currentStatsLocal());

	return true;
}

void Statistics::run()
{
	static auto last{ std::chrono::steady_clock::now() };

	const auto now{ std::chrono::steady_clock::now() };
	const double dt{ std::chrono::duration<double>(now - last).count() };

	last = now;

	//long gaps (a breakpoint, a hitch while loading) aren't time spent playing
	tick(std::min(dt, 1.0), currentStatsLocal());
}

void Statistics::setActive(const bool active)
{
	m_active = active;

	//saved straight away so it sticks even if the game closes before the next save
	save();
}

StatsSnapshot Statistics::snapshot() const
{
	const std::scoped_lock lock{ m_mutex };

	return StatsSnapshot{ m_all_time, m_session, m_per_class, m_per_map, m_per_weapon, m_life_active ? m_life_kills : 0, m_life_active ? m_life_time : 0.0 };
}

void Statistics::resetSession()
{
	const std::scoped_lock lock{ m_mutex };

	m_session = {};
}

void Statistics::resetAll()
{
	const std::scoped_lock lock{ m_mutex };

	m_all_time = {};
	m_session = {};
	m_per_class = {};
	m_per_map.clear();
	m_per_weapon.clear();

	m_dirty = true;
	saveLocked();
}
