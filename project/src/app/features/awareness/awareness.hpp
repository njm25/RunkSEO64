#pragma once

#include "../../../game/game.hpp"

//things worth knowing about the enemy team: notable items, respawns, pack timers, sentry and sticky danger, snipers aiming at you
class Awareness final : public HasLevelInit, public HasLevelShutdown
{
private:
	struct Pack final
	{
		vec3 origin{};
		bool health{};
		float taken_time{}; //0 while it's there
		float last_seen{};
	};

private:
	std::unordered_map<int, Pack> m_packs{}; //by entindex, map placed packs only
	std::unordered_set<int> m_pack_models_health{};
	std::unordered_set<int> m_pack_models_ammo{};

private:
	void cachePackModels();
	void updatePacks();

	void paintCallouts();
	void paintRespawnTimers();
	void paintPackTimers();
	void paintWarnings();

	void renderSentryRanges();
	void renderStickyZones();

public:
	bool onLevelInit() override;
	bool onLevelShutdown() override;

public:
	void paint(); //2d, from the in game panels paint
	void render(); //3d, from the world draw
};

MAKE_UNIQUE(Awareness, awareness);
