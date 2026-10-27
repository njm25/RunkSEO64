#pragma once

#include "../../../game/game.hpp"

#include <array>
#include <map>
#include <mutex>

enum class StatGroup
{
	COMBAT, DAMAGE, CLASS, OBJECTIVES, RECORDS, TIME
};

enum class StatKind
{
	COUNT,  //adds up
	RECORD, //keeps the highest value
	TIME    //seconds, adds up
};

enum class Stat
{
	//combat
	KILLS, DEATHS, ASSISTS, HEADSHOTS, BACKSTABS, MELEE_KILLS, CRIT_KILLS, MINICRIT_KILLS,
	DOMINATIONS, ASSISTED_DOMINATIONS, REVENGES, ASSISTED_REVENGES, TIMES_DOMINATED, TIMES_AVENGED_ON,
	FIRST_BLOODS, GIBS, TAUNT_KILLS, TELEFRAGS, ENVIRONMENTAL_DEATHS, SUICIDES, FEIGN_DEATHS,

	//damage
	DAMAGE_DEALT, DAMAGE_TAKEN, CRIT_DAMAGE, MINICRIT_DAMAGE, BIGGEST_HIT, SELF_DAMAGE,

	//class specific
	HEALING, UBERS, MEDICS_KILLED, CHARGED_MEDICS_KILLED, SENTRIES_DESTROYED, DISPENSERS_DESTROYED, TELEPORTERS_DESTROYED,
	OWN_BUILDINGS_LOST, BUILDINGS_BUILT, TEAMMATES_TELEPORTED, REFLECTS, EXTINGUISHES, IGNITES, JARATES, STUNS, SAPPERS,

	//objectives
	CAPTURES, CAPTURES_BLOCKED, FLAG_PICKUPS, FLAG_CAPTURES, FLAG_DEFENDS, ROUNDS_WON, ROUNDS_LOST,

	//records
	BEST_KILLSTREAK, LONGEST_LIFE, MOST_ROUND_KILLS, MOST_LIFE_DAMAGE,

	//time
	TIME_IN_GAME, SESSIONS, TIME_LOADED,

	COUNT
};

struct StatInfo final
{
	const char *id{}; //key in stats.json, never changes
	const char *label{};
	StatGroup group{};
	StatKind kind{};
};

const StatInfo &statInfo(const Stat stat);

using StatBlock = std::array<double, static_cast<size_t>(Stat::COUNT)>;

//what the tracker needs to know about you when something happens
struct StatsLocal final
{
	bool in_game{};
	int userid{};
	int entindex{};
	int team{};
	int player_class{}; //1-9, 0 when unknown
	bool alive{};
	bool melee{}; //holding the melee weapon
	std::string map{};
};

//filled in from the game by statistics_game.cpp
StatsLocal currentStatsLocal();

constexpr int stats_class_count{ 10 }; //indexed by tf class, 0 unused

//a copy of everything, for drawing without holding the lock
struct StatsSnapshot final
{
	StatBlock all_time{};
	StatBlock session{};
	std::array<StatBlock, stats_class_count> per_class{};
	std::map<std::string, StatBlock> per_map{};
	std::map<std::string, double> per_weapon{}; //kills by kill icon name

	int current_streak{};
	double current_life{};
};

class Statistics final : public HasLoad, public HasUnload, public HasGameEvent
{
private:
	mutable std::mutex m_mutex{};

	StatBlock m_all_time{};
	StatBlock m_session{};
	std::array<StatBlock, stats_class_count> m_per_class{};
	std::map<std::string, StatBlock> m_per_map{};
	std::map<std::string, double> m_per_weapon{};

	//current life and round
	bool m_life_active{};
	double m_life_time{};
	int m_life_kills{};
	double m_life_damage{};
	int m_round_kills{};

	double m_save_timer{};
	bool m_dirty{};

private:
	void add(const Stat stat, const double value, const StatsLocal &local);
	void record(const Stat stat, const double value, const StatsLocal &local);
	void startLife();
	void endLife(const StatsLocal &local);
	void endRound(const StatsLocal &local);

	void saveLocked() const;

public:
	bool onLoad() override;
	bool onUnload() override;
	bool onGameEvent(IGameEvent *const event) override;

public:
	//the parts that don't touch the game, so they can be fed directly
	void handleEvent(IGameEvent *const event, const StatsLocal &local);
	void tick(const double dt, const StatsLocal &local);

	void run(); //once a frame
	void save() const;
	void load();

	StatsSnapshot snapshot() const;
	void resetSession();
	void resetAll();
};

MAKE_UNIQUE(Statistics, statistics);
