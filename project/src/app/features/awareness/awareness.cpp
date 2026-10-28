#include "awareness.hpp"

#include "../vis_utils/vis_utils.hpp"
#include "../trace_utils/trace_utils.hpp"
#include "../statistics/statistics.hpp"
#include "../cfg.hpp"

static constexpr float sentry_range{ 1100.0f };
static constexpr float pack_respawn_time{ 10.0f };
static constexpr float sticky_default_radius{ 146.0f };

static const Color danger_clr{ 255, 60, 60, 255 };

static bool visibleFrom(const vec3 &from, const vec3 &to)
{
	trace_t tr{};
	tf_utils::trace(from, to, &trace_filters::world, &tr, MASK_SOLID);

	return tr.fraction > 0.99f;
}

//flat circle made of lines, for ranges on the ground
static void ring(const vec3 &center, const float radius, const Color clr)
{
	constexpr int segments{ 48 };

	vec3 prev{ center.x + radius, center.y, center.z };

	for (int n{ 1 }; n <= segments; n++)
	{
		const float a{ static_cast<float>(n) / static_cast<float>(segments) * 6.2831853f };
		const vec3 cur{ center.x + std::cos(a) * radius, center.y + std::sin(a) * radius, center.z };

		render_utils::RenderLine(prev, cur, clr, true);
		prev = cur;
	}
}

//==================================================================================================================================
//callouts

//items that change how you should fight someone, matched against the names in weapon_names.cpp
static const char *calloutFor(const int def_index)
{
	static std::unordered_map<int, const char *> cache{};

	if (const auto it{ cache.find(def_index) }; it != cache.end()) {
		return it->second;
	}

	static constexpr std::pair<std::string_view, const char *> callouts[]
	{
		{ "Dead Ringer", "DEAD RINGER" },
		{ "Cloak and Dagger", "C&D" },
		{ "Your Eternal Reward", "YER" },
		{ "Wanga Prick", "YER" },
		{ "Kritzkrieg", "KRITZ" },
		{ "Quick Fix", "QUICK-FIX" },
		{ "Vaccinator", "VACC" },
		{ "Ubersaw", "UBERSAW" },
		{ "Gunslinger", "GUNSLINGER" },
		{ "Wrangler", "WRANGLER" },
		{ "Short Circuit", "SHORT CIRCUIT" },
		{ "Sydney Sleeper", "SLEEPER" },
		{ "Huntsman", "HUNTSMAN" },
		{ "Fortified Compound", "HUNTSMAN" },
		{ "Razorback", "RAZORBACK" },
		{ "Darwin's Danger Shield", "DDS" },
		{ "Phlogistinator", "PHLOG" },
		{ "Natascha", "NATASCHA" },
		{ "Targe", "SHIELD" },
		{ "Splendid Screen", "SHIELD" },
		{ "Tide Turner", "SHIELD" },
		{ "Battalion's Backup", "BATTALIONS" },
		{ "Bonk!", "BONK" }
	};

	const char *out{};

	if (const char *const name{ weaponName(def_index) })
	{
		const std::string_view str{ name };

		for (const auto &[match, label] : callouts)
		{
			if (str.find(match) != std::string_view::npos) {
				out = label;
				break;
			}
		}
	}

	cache[def_index] = out;

	return out;
}

void Awareness::paintCallouts()
{
	if (!cfg::callouts_active) {
		return;
	}

	for (const EntityHandle_t ehandle : ec->getGroup(ECGroup::PLAYERS_ENEMIES))
	{
		IClientEntity *const ent{ i::ent_list->GetClientEntityFromHandle(ehandle) };

		if (!ent) {
			continue;
		}

		C_TFPlayer *const pl{ ent->cast<C_TFPlayer>() };

		if (!pl || pl->deadflag()) {
			continue;
		}

		std::string text{};

		for (int n{}; n < MAX_WEAPONS; n++)
		{
			C_TFWeaponBase *const weapon{ pl->GetWeapon(n) };

			if (!weapon) {
				continue;
			}

			const char *const callout{ calloutFor(weapon->m_iItemDefinitionIndex()) };

			if (!callout || text.find(callout) != std::string::npos) {
				continue;
			}

			text += text.empty() ? "" : " / ";
			text += callout;
		}

		vec2 screen{};

		if (text.empty() || !draw->worldToScreen(pl->m_vecOrigin(), screen)) {
			continue;
		}

		//under the feet, the name and the rest of the esp sit above the head
		draw->stringOutlined({ screen.x, screen.y + 2.0f }, fonts::esp.get(), text, cfg::callouts_color, POS_CENTERX);
	}
}

//==================================================================================================================================
//respawn timers

void Awareness::paintRespawnTimers()
{
	if (!cfg::respawn_timers_active) {
		return;
	}

	C_TFPlayer *const local{ ec->getLocal() };
	C_TFPlayerResource *const res{ ec->getPlayerResource() };

	if (!local || !res) {
		return;
	}

	struct Entry final
	{
		std::string text{};
		float time{};
		Color clr{};
	};

	std::vector<Entry> entries{};

	int enemies_down{};
	float first_enemy_back{ FLT_MAX };

	for (int n{ 1 }; n <= i::global_vars->maxClients; n++)
	{
		if (!res->GetConnected(n) || res->GetAlive(n)) {
			continue;
		}

		const int team{ res->GetTeam(n) };

		if (team != TF_TEAM_RED && team != TF_TEAM_BLU) {
			continue;
		}

		const bool enemy{ team != local->m_iTeamNum() };

		if (!enemy && !cfg::respawn_timers_teammates) {
			continue;
		}

		const float left{ res->GetNextRespawnTime(n) - i::global_vars->curtime };

		//0 before the server sets it, and modes like saxton hale use huge values
		if (left < -1.0f || left > 60.0f) {
			continue;
		}

		player_info_t info{};

		if (!i::engine->GetPlayerInfo(n, &info)) {
			continue;
		}

		if (enemy) {
			enemies_down++;
			first_enemy_back = std::min(first_enemy_back, left);
		}

		entries.push_back(Entry
		{
			std::format("{:.0f}s  {} ({})", std::max(std::ceil(left), 0.0f), info.name, vis_utils->getClassName(res->GetPlayerClass(n))),
			left,
			enemy ? Color{ 255, 130, 130, 255 } : Color{ 130, 170, 255, 255 }
		});
	}

	if (entries.empty()) {
		return;
	}

	std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) { return a.time < b.time; });

	const float tall{ fonts::indicators.getTall() + 2.0f };
	vec2 pos{ 10.0f, draw->getScreenSize().y * cfg::respawn_timers_pos };

	if (enemies_down)
	{
		draw->stringOutlined(pos, fonts::indicators.get(), std::format("{} enemies down, first back in {:.0f}s", enemies_down, std::max(std::ceil(first_enemy_back), 0.0f)), { 230, 230, 230, 255 });
		pos.y += tall;
	}

	for (const Entry &entry : entries)
	{
		draw->stringOutlined(pos, fonts::indicators.get(), entry.text, entry.clr);
		pos.y += tall;
	}
}

//==================================================================================================================================
//pack timers

void Awareness::cachePackModels()
{
	m_pack_models_health.clear();
	m_pack_models_ammo.clear();

	//only map placed packs respawn, dropped ammo and sandviches don't
	for (const char *const mdl : {
		"models/items/medkit_small.mdl", "models/items/medkit_medium.mdl", "models/items/medkit_large.mdl",
		"models/items/medkit_small_bday.mdl", "models/items/medkit_medium_bday.mdl", "models/items/medkit_large_bday.mdl",
		"models/props_halloween/halloween_medkit_small.mdl", "models/props_halloween/halloween_medkit_medium.mdl", "models/props_halloween/halloween_medkit_large.mdl",
		"models/props_medieval/medieval_meat.mdl" })
	{
		if (const int idx{ i::model_info->GetModelIndex(mdl) }; idx > 0) {
			m_pack_models_health.insert(idx);
		}
	}

	for (const char *const mdl : {
		"models/items/ammopack_small.mdl", "models/items/ammopack_medium.mdl", "models/items/ammopack_large.mdl",
		"models/items/ammopack_small_bday.mdl", "models/items/ammopack_medium_bday.mdl", "models/items/ammopack_large_bday.mdl" })
	{
		if (const int idx{ i::model_info->GetModelIndex(mdl) }; idx > 0) {
			m_pack_models_ammo.insert(idx);
		}
	}
}

void Awareness::updatePacks()
{
	if (m_pack_models_health.empty() && m_pack_models_ammo.empty()) {
		cachePackModels();
	}

	C_TFPlayer *const local{ ec->getLocal() };

	if (!local) {
		return;
	}

	const float now{ i::global_vars->curtime };

	for (const ECGroup group : { ECGroup::HEALTH_PACKS, ECGroup::AMMO_PACKS })
	{
		for (const EntityHandle_t ehandle : ec->getGroup(group))
		{
			IClientEntity *const ent{ i::ent_list->GetClientEntityFromHandle(ehandle) };

			if (!ent) {
				continue;
			}

			C_BaseEntity *const pack{ ent->cast<C_BaseEntity>() };
			const int model{ pack->m_nModelIndex() };
			const bool health{ m_pack_models_health.contains(model) };

			if (!health && !m_pack_models_ammo.contains(model)) {
				continue;
			}

			m_packs[pack->entindex()] = Pack{ pack->GetCenter(), health, 0.0f, now };
		}
	}

	//a picked up pack stops being sent to clients, so it goes dormant. so does one that's just out of view,
	//so it only counts as taken when it vanished right in front of you. coming round a corner, the spot is visible
	//a moment before the pack arrives, which the recent sighting rules out
	const vec3 eye{ local->GetEyePos() };

	for (auto it{ m_packs.begin() }; it != m_packs.end();)
	{
		Pack &pack{ it->second };
		IClientEntity *const ent{ i::ent_list->GetClientEntity(it->first) };

		if (ent && !ent->IsDormant())
		{
			//the index got reused by something else
			const int model{ ent->cast<C_BaseEntity>()->m_nModelIndex() };

			if (!m_pack_models_health.contains(model) && !m_pack_models_ammo.contains(model)) {
				it = m_packs.erase(it);
				continue;
			}

			++it;
			continue;
		}

		if (pack.taken_time > 0.0f)
		{
			//respawned without us seeing it come back
			if (now - pack.taken_time > pack_respawn_time + 1.0f) {
				pack.taken_time = 0.0f;
			}
		}

		else if (now - pack.last_seen < 0.25f && visibleFrom(eye, pack.origin)) {
			pack.taken_time = pack.last_seen;
		}

		++it;
	}
}

void Awareness::paintPackTimers()
{
	if (!cfg::pack_timers_active) {
		return;
	}

	updatePacks();

	const float now{ i::global_vars->curtime };

	for (const auto &[idx, pack] : m_packs)
	{
		if (pack.taken_time <= 0.0f) {
			continue;
		}

		vec2 screen{};

		if (!draw->worldToScreen(pack.origin, screen)) {
			continue;
		}

		const float left{ std::max(pack_respawn_time - (now - pack.taken_time), 0.0f) };
		const Color clr{ pack.health ? Color{ 120, 255, 120, 255 } : Color{ 230, 230, 120, 255 } };

		draw->stringOutlined(screen, fonts::esp.get(), std::format("{} {:.0f}s", pack.health ? "hp" : "ammo", std::ceil(left)), clr, POS_CENTERXY);
	}
}

//==================================================================================================================================
//sentries, stickies, snipers

static vec3 sentryEye(C_ObjectSentrygun *const sentry)
{
	const int level{ sentry->m_bMiniBuilding() ? 1 : sentry->m_iUpgradeLevel() };

	return sentry->m_vecOrigin() + vec3{ 0.0f, 0.0f, level >= 3 ? 46.0f : level == 2 ? 40.0f : 32.0f };
}

static bool sentryCanShoot(C_ObjectSentrygun *const sentry)
{
	return !sentry->m_bPlacing() && !sentry->m_bCarried() && !sentry->m_bBuilding() && !sentry->IsDisabled();
}

static bool sentryThreat(C_ObjectSentrygun *const sentry, const vec3 &target)
{
	const vec3 eye{ sentryEye(sentry) };

	return sentryCanShoot(sentry) && eye.distTo(target) <= sentry_range && visibleFrom(eye, target);
}

static float stickyRadius(C_TFGrenadePipebombProjectile *const sticky)
{
	const float radius{ sticky->m_DmgRadius() };

	return radius > 1.0f ? radius : sticky_default_radius;
}

static bool stickyThreat(C_TFGrenadePipebombProjectile *const sticky, const vec3 &target)
{
	//not armed yet, it can't be set off
	if (i::global_vars->curtime - sticky->m_flCreationTime() < cvars::tf_grenadelauncher_livetime->GetFloat()) {
		return false;
	}

	const vec3 pos{ sticky->GetCenter() };

	return pos.distTo(target) <= stickyRadius(sticky) && visibleFrom(pos, target);
}

template <typename T>
static void forEachEnemy(const ECGroup group, const ClassId &class_id, const std::function<void(T *const)> &fn)
{
	for (const EntityHandle_t ehandle : ec->getGroup(group))
	{
		IClientEntity *const ent{ i::ent_list->GetClientEntityFromHandle(ehandle) };

		if (!ent || !(class_id == ent->GetClassId())) {
			continue;
		}

		if (T *const t{ ent->cast<T>() }) {
			fn(t);
		}
	}
}

static void forEachSticky(const std::function<void(C_TFGrenadePipebombProjectile *const)> &fn)
{
	forEachEnemy<C_TFGrenadePipebombProjectile>(ECGroup::PROJECTILES_ENEMIES, class_ids::CTFGrenadePipebombProjectile, [&](C_TFGrenadePipebombProjectile *const sticky)
	{
		if (sticky->HasStickyEffects()) {
			fn(sticky);
		}
	});
}

static bool sniperAimingAt(C_TFPlayer *const sniper, C_TFPlayer *const target)
{
	if (sniper->deadflag() || sniper->m_iClass() != TF_CLASS_SNIPER) {
		return false;
	}

	C_TFWeaponBase *const weapon{ sniper->m_hActiveWeapon()->cast<C_TFWeaponBase>() };

	if (!weapon) {
		return false;
	}

	//same idea of "scoped in" as the sight lines
	if (weapon->GetWeaponID() == TF_WEAPON_SNIPERRIFLE_CLASSIC)
	{
		if (!weapon->cast<C_TFSniperRifleClassic>()->m_bCharging()) {
			return false;
		}
	}

	else if (!sniper->InCond(TF_COND_ZOOMED)) {
		return false;
	}

	vec3 dir{};
	math::angleVectors(sniper->m_angEyeAngles(), &dir);

	const vec3 from{ sniper->m_vecOrigin() + vec3{ 0.0f, 0.0f, ((sniper->m_fFlags() & FL_DUCKING) ? 45.0f : 75.0f) * sniper->m_flModelScale() } };
	const vec3 center{ target->GetCenter() };
	const float along{ (center - from).dot(dir) };

	if (along <= 0.0f) {
		return false;
	}

	//how far the aim line passes from your body
	if ((from + dir * along).distTo(center) > 40.0f) {
		return false;
	}

	return visibleFrom(from, center);
}

void Awareness::paintWarnings()
{
	C_TFPlayer *const local{ ec->getLocal() };

	if (!local || local->deadflag()) {
		return;
	}

	const vec3 center{ local->GetCenter() };

	std::vector<std::string> lines{};

	if (cfg::sentry_ranges_active && cfg::sentry_warning)
	{
		int sentries{};

		forEachEnemy<C_ObjectSentrygun>(ECGroup::BUILDINGS_ENEMIES, class_ids::CObjectSentrygun, [&](C_ObjectSentrygun *const sentry)
		{
			if (sentryThreat(sentry, center)) {
				sentries++;
			}
		});

		if (sentries) {
			lines.push_back(sentries > 1 ? std::format("{} sentries can see you", sentries) : "a sentry can see you");
		}
	}

	if (cfg::sticky_zones_active && cfg::sticky_warning)
	{
		int stickies{};

		forEachSticky([&](C_TFGrenadePipebombProjectile *const sticky)
		{
			if (stickyThreat(sticky, center)) {
				stickies++;
			}
		});

		if (stickies) {
			lines.push_back(std::format("in range of {} {}", stickies, stickies > 1 ? "stickies" : "sticky"));
		}
	}

	if (cfg::sniper_warning)
	{
		for (const EntityHandle_t ehandle : ec->getGroup(ECGroup::PLAYERS_ENEMIES))
		{
			IClientEntity *const ent{ i::ent_list->GetClientEntityFromHandle(ehandle) };

			if (!ent) {
				continue;
			}

			C_TFPlayer *const sniper{ ent->cast<C_TFPlayer>() };
			player_info_t info{};

			if (sniper && sniperAimingAt(sniper, local) && i::engine->GetPlayerInfo(sniper->entindex(), &info)) {
				lines.push_back(std::format("sniper aiming at you: {}", info.name));
			}
		}
	}

	const vec2 screen{ draw->getScreenSize() };
	vec2 pos{ screen.x * 0.5f, screen.y * 0.62f };

	for (const std::string &line : lines)
	{
		draw->stringOutlined(pos, fonts::indicators.get(), line, danger_clr, POS_CENTERX);
		pos.y += fonts::indicators.getTall() + 2.0f;
	}
}

void Awareness::renderSentryRanges()
{
	if (!cfg::sentry_ranges_active) {
		return;
	}

	C_TFPlayer *const local{ ec->getLocal() };

	if (!local) {
		return;
	}

	const vec3 center{ local->GetCenter() };

	forEachEnemy<C_ObjectSentrygun>(ECGroup::BUILDINGS_ENEMIES, class_ids::CObjectSentrygun, [&](C_ObjectSentrygun *const sentry)
	{
		if (sentry->m_bPlacing() || sentry->m_bCarried()) {
			return;
		}

		const vec3 origin{ sentry->m_vecOrigin() };

		if (cfg::sentry_ranges_only_near && origin.distTo(center) > sentry_range * 1.5f) {
			return;
		}

		Color clr{ !local->deadflag() && sentryThreat(sentry, center) ? danger_clr : vis_utils->getEntColor(sentry) };

		//a sapped or unfinished sentry can't shoot, fade it
		clr.a = sentryCanShoot(sentry) ? 200 : 70;

		ring(origin + vec3{ 0.0f, 0.0f, 4.0f }, sentry_range, clr);
	});
}

void Awareness::renderStickyZones()
{
	if (!cfg::sticky_zones_active) {
		return;
	}

	C_TFPlayer *const local{ ec->getLocal() };

	if (!local) {
		return;
	}

	const vec3 center{ local->GetCenter() };

	forEachSticky([&](C_TFGrenadePipebombProjectile *const sticky)
	{
		Color clr{ !local->deadflag() && stickyThreat(sticky, center) ? danger_clr : vis_utils->getEntColor(sticky) };
		clr.a = 180;

		ring(sticky->GetCenter(), stickyRadius(sticky), clr);
	});
}

//==================================================================================================================================

bool Awareness::onLevelInit()
{
	m_packs.clear();
	m_pack_models_health.clear();
	m_pack_models_ammo.clear();

	return true;
}

bool Awareness::onLevelShutdown()
{
	return onLevelInit();
}

void Awareness::paint()
{
	if (!vis_utils->shouldRunVisuals()) {
		return;
	}

	paintCallouts();
	paintRespawnTimers();
	paintPackTimers();
	paintWarnings();
}

void Awareness::render()
{
	if (!vis_utils->shouldRunVisuals()) {
		return;
	}

	renderSentryRanges();
	renderStickyZones();
}
