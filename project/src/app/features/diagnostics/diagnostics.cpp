#include "diagnostics.hpp"

#include "../notifs/notifs.hpp"

#include <build_info.hpp>

const char *loadKindName(const LoadKind kind)
{
	switch (kind)
	{
		case LoadKind::SIG: return "sig";
		case LoadKind::IFACE: return "interface";
		case LoadKind::CVAR: return "cvar";
		case LoadKind::HOOK: return "hook";
		case LoadKind::NETVAR: return "netvar";
		case LoadKind::NETVAR_HOOK: return "netvar hook";
	}

	return "?";
}

//what each setting needs to work, a var ending in * is a prefix
struct Requirement final
{
	std::string_view var{};
	std::vector<std::string_view> hooks{};
	std::vector<std::string_view> sigs{};
};

static const std::vector<Requirement> &requirements()
{
	static const std::vector<Requirement> reqs
	{
		//everything run from createmove
		{ "aimbot_*", { "CHLClient_CreateMove" } },
		{ "auto_*", { "CHLClient_CreateMove" } },
		{ "tb_*", { "CHLClient_CreateMove" } },
		{ "antiaim_*", { "CHLClient_CreateMove" } },
		{ "fakelag_*", { "CHLClient_CreateMove" } },
		{ "crits_*", { "CHLClient_CreateMove" } },
		{ "seed_pred_*", { "CHLClient_CreateMove" } },
		{ "misc_bhop", { "CHLClient_CreateMove" } },
		{ "misc_faststop", { "CHLClient_CreateMove" } },

		{ "allow_mvm_inspect", { "ClientModeTFNormal_BIsFriendOrPartyMember" } },
		{ "aspect_ratio_*", { "IVEngineClient_GetScreenAspectRatio" } },
		{ "auto_accept_*", { "CMatchInviteNotification_OnTick" } },
		{ "misc_remove_autojoin_countdown", { "CMatchInviteNotification_OnTick" }, { "CMatchInviteNotification_Flags_Offset" } },
		{ "crosshair_follows_aimbot", { "CHudCrosshair_GetDrawPosition" } },
		{ "flat_textures", { "CMaterialSystem_FindTexture" } },
		{ "force_low_lod_models", { "CModelRender_DrawModelExecute" } },
		{ "remove_hands", { "CModelRender_DrawModelExecute" } },
		{ "fov_override_*", { "C_BasePlayer_GetFOV" } },
		{ "misc_bypass_f2p_chat", { "CTFGCClientSystem_UpdateAssignedLobby" }, { "GTFGCClientSystem_NonPremiumAccount_Offset" } },
		{ "misc_damage_feedback_*", { "CDamageAccountPanel_DisplayDamageFeedback" } },
		{ "misc_damage_feedback_show_through_walls", { "CDamageAccountPanel_DisplayDamageFeedback", "MainViewForward" } },
		{ "misc_expanded_inventory", { "CTFPlayerInventory_GetMaxItemCount" } },
		{ "misc_giant_weapon_sounds", { "C_TFWeaponBase_GetShootSound" } },
		{ "misc_medieval_chat", { "CTFGameRules_ModifySentChat" } },
		{ "misc_owo_chat", { "IVEngineClient_ClientCmd_Unrestricted" } },
		{ "misc_rainbow_party", { "CTFPartyClient_GetPartyMemberColor" } },
		{ "misc_remove_end_of_match_surveys", { "CTFMatchSumary_OnTick" }, { "GTFGCClientSystem", "CMsgSurveyRequest_match_id_Offset" } },
		{ "misc_remove_equip_regions", { "CEconItemSchema_GetItemDefinition" } },
		{ "misc_remove_fix_angle", { "CClientState_ProcessFixAngle" } },
		{ "misc_remove_teammate_pushaway", { "C_TFPlayer_CreateMove" } },
		{ "misc_remove_item_found_notification", { "NotificationQueue_Add", "CInventoryManager_ShowItemsPickedUp" } },
		{ "misc_sv_pure_bypass", { "CL_CheckForPureServerWhitelist", "CNetChan_SendNetMsg" } },
		{ "particle_mod_*", { "C_OP_RenderSprites_RenderSpriteCard" } },
		{ "remove_fade", { "CUserMessages_DispatchUserMessage" } },
		{ "remove_shake", { "CUserMessages_DispatchUserMessage" } },
		{ "remove_fog", { "CRendering3dView_EnableWorldFog" } },
		{ "remove_post_processing", { "DoEnginePostProcessing" } },
		{ "remove_punch", { "C_BasePlayer_CalcPlayerView" } },
		{ "remove_scope", { "CTFPlayerShared_InCond" } },
		{ "remove_sky_fog", { "CSkyboxView_GetSkyboxFogEnable" } },
		{ "remove_interp", { "C_BaseAnimating_Interpolate", "C_TFPlayer_UpdateClientSideAnimation" } },
		{ "reveal_scoreboard", { "CTFPlayerShared_IsPlayerDominated", "KeyValues_SetInt" } },
		{ "color_scoreboard", { "C_PlayerResource_GetTeamColor", "C_TFPlayerResource_GetPlayerConnectionState" } },
		{ "skybox_override", { "R_LoadNamedSkys" } },
		{ "spritecard_texture_hack", { "CShaderAPIDx8_BindStandardTexture" } },
		{ "static_player_model_lighting", { "CModelRender_StudioSetupLighting" } },
		{ "tp_override_*", { "ClientModeShared_OverrideView" } },
		{ "tracers_*", { "UTIL_ParticleTracer" } },
		{ "tracers_active", { "C_TFPlayer_FireBullet", "UTIL_ParticleTracer", "WeaponID_IsSniperRifle" } },
		{ "vm_override_*", { "C_TFViewModel_CalcViewModelView" } }
	};

	return reqs;
}

static bool requirementMatches(std::string_view pattern, std::string_view var)
{
	if (pattern.ends_with('*')) {
		return var.starts_with(pattern.substr(0, pattern.size() - 1));
	}

	return pattern == var;
}

void Diagnostics::computeUnavailable()
{
	m_unavailable.clear();

	const std::vector<LoadItem> items{ load_report->items() };

	auto failure = [&](const LoadKind kind, std::string_view name) -> const LoadItem *
	{
		for (const LoadItem &item : items)
		{
			if (item.kind == kind && item.name == name) {
				return item.ok ? nullptr : &item;
			}
		}

		return nullptr;
	};

	for (ConfigVar *const var : getInsts<ConfigVar>())
	{
		if (!var) {
			continue;
		}

		std::vector<std::string> reasons{};

		for (const Requirement &req : requirements())
		{
			if (!requirementMatches(req.var, var->name())) {
				continue;
			}

			for (const std::string_view hook : req.hooks)
			{
				if (const LoadItem *const item{ failure(LoadKind::HOOK, hook) }) {
					reasons.push_back(std::format("hook {} failed ({})", hook, item->detail));
				}
			}

			for (const std::string_view sig : req.sigs)
			{
				if (failure(LoadKind::SIG, sig)) {
					reasons.push_back(std::format("sig {} not found", sig));
				}
			}
		}

		if (reasons.empty()) {
			continue;
		}

		//a prefix and an exact rule can both name the same hook
		std::sort(reasons.begin(), reasons.end());
		reasons.erase(std::unique(reasons.begin(), reasons.end()), reasons.end());

		std::string reason{};

		for (const std::string &r : reasons) {
			reason += (reason.empty() ? "" : "\n") + r;
		}

		m_unavailable[var->name()] = reason;
	}
}

static std::string gameVersionPath()
{
	return utils::getFilePath() + "game_version.json";
}

std::vector<std::string> Diagnostics::checkGameVersion(const std::vector<ModuleStamp> &now)
{
	std::vector<std::string> changed{};

	nlohmann::json prev{};

	if (std::ifstream file{ gameVersionPath() }; file.is_open())
	{
		try {
			file >> prev;
		}

		catch (...) {
			prev = {};
		}
	}

	nlohmann::json j{};

	for (const ModuleStamp &mod : now)
	{
		j["modules"][mod.name] = { { "timestamp", mod.timestamp }, { "size", mod.size } };

		//modules seen for the first time don't count, otherwise the very first load would say the game updated
		if (!prev.is_object() || !prev.contains("modules") || !prev["modules"].contains(mod.name)) {
			continue;
		}

		const nlohmann::json &old{ prev["modules"][mod.name] };

		if (old.value("timestamp", 0u) != mod.timestamp || old.value("size", 0u) != mod.size) {
			changed.push_back(mod.name);
		}
	}

	if (std::ofstream file{ gameVersionPath() }; file.is_open()) {
		file << std::setw(4) << j;
	}

	return changed;
}

void Diagnostics::finishLoad()
{
	computeUnavailable();

	m_modules = diag_platform::moduleStamps();
	m_changed_modules = checkGameVersion(m_modules);

	const size_t failed{ failedCount() };

	m_failed_at_load = failed;

	if (gameUpdated() && failed) {
		notifs->warning(std::format("TF2 updated, {} sigs/hooks failed, see the diagnostics tab", failed));
		m_open_requested = true;
	}

	else if (failed) {
		notifs->warning(std::format("{} sigs/hooks failed to load, see the diagnostics tab", failed));
	}

	else if (gameUpdated()) {
		notifs->message("TF2 updated, everything still loaded fine");
	}
}

std::string Diagnostics::unavailableReason(const ConfigVar *const var) const
{
	if (!var || m_unavailable.empty()) {
		return {};
	}

	const auto it{ m_unavailable.find(var->name()) };

	return it == m_unavailable.end() ? std::string{} : it->second;
}

const std::unordered_map<std::string, std::string> &Diagnostics::unavailable() const
{
	return m_unavailable;
}

bool Diagnostics::gameUpdated() const
{
	return !m_changed_modules.empty();
}

const std::vector<std::string> &Diagnostics::changedModules() const
{
	return m_changed_modules;
}

const std::vector<ModuleStamp> &Diagnostics::modules() const
{
	return m_modules;
}

bool Diagnostics::consumeOpenRequest()
{
	return std::exchange(m_open_requested, false);
}

size_t Diagnostics::failedCount() const
{
	size_t count{};

	for (const LoadItem &item : load_report->items())
	{
		//netvars are looked up as they're used, those get their own count in the tab
		if (!item.ok && item.kind != LoadKind::NETVAR) {
			count++;
		}
	}

	return count;
}

size_t Diagnostics::failedAtLoad() const
{
	return m_failed_at_load;
}

std::string Diagnostics::formatAddress(const LoadItem &item) const
{
	if (!item.ok || !item.addr) {
		return "-";
	}

	//netvars are offsets into the entity, not addresses
	if (item.kind == LoadKind::NETVAR) {
		return std::format("+0x{:X}", item.addr);
	}

	auto it{ m_offsets.find(item.addr) };

	if (it == m_offsets.end()) {
		it = m_offsets.emplace(item.addr, diag_platform::moduleOf(item.addr)).first;
	}

	if (it->second.module.empty()) {
		return std::format("0x{:X}", item.addr);
	}

	return std::format("{}+0x{:X}", it->second.module, it->second.offset);
}

std::string Diagnostics::report() const
{
	const std::vector<LoadItem> items{ load_report->items() };

	std::string out{ std::format("Runkseo64 build {} ({})\n", BUILD_NUMBER, BUILD_HASH) };

	if (gameUpdated())
	{
		out += "game updated since last load:";

		for (const std::string &mod : m_changed_modules) {
			out += " " + mod;
		}

		out += "\n";
	}

	out += "\nmodules:\n";

	for (const ModuleStamp &mod : m_modules) {
		out += std::format("  {} timestamp 0x{:08X} size 0x{:X}\n", mod.name, mod.timestamp, mod.size);
	}

	//failures first, they're what anyone reading this is after
	for (const bool ok : { false, true })
	{
		out += ok ? "\nloaded:\n" : "\nfailed:\n";

		size_t count{};

		for (const LoadItem &item : items)
		{
			if (item.ok != ok) {
				continue;
			}

			out += std::format("  [{}] {} {}", loadKindName(item.kind), item.name, formatAddress(item));

			if (item.ms > 0.0) {
				out += std::format(" {:.2f}ms", item.ms);
			}

			if (!item.detail.empty()) {
				out += " - " + item.detail;
			}

			out += "\n";
			count++;
		}

		if (!count) {
			out += "  none\n";
		}
	}

	if (!m_unavailable.empty())
	{
		std::vector<std::pair<std::string, std::string>> sorted{ m_unavailable.begin(), m_unavailable.end() };
		std::sort(sorted.begin(), sorted.end());

		out += "\nunavailable settings:\n";

		for (const auto &[var, reason] : sorted)
		{
			std::string one_line{ reason };
			std::replace(one_line.begin(), one_line.end(), '\n', ';');

			out += std::format("  {} - {}\n", var, one_line);
		}
	}

	return out;
}
