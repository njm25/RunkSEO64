#include "menu_context.hpp"

#include "../cfg.hpp"

enum ContextClass
{
	CLASS_NONE, CLASS_SCOUT, CLASS_SNIPER, CLASS_SOLDIER, CLASS_DEMOMAN, CLASS_MEDIC, CLASS_HEAVY, CLASS_PYRO, CLASS_SPY, CLASS_ENGINEER
};

void ContextHighlight::update(const MenuContext &ctx)
{
	m_ctx = ctx;
}

bool ContextHighlight::active() const
{
	return cfg::ui_context_highlight && m_ctx.in_game && m_ctx.player_class != CLASS_NONE;
}

const MenuContext &ContextHighlight::context() const
{
	return m_ctx;
}

std::string ContextHighlight::describe() const
{
	static constexpr const char *const class_names[]{ "", "scout", "sniper", "soldier", "demoman", "medic", "heavy", "pyro", "spy", "engineer" };
	static constexpr const char *const weapon_names[]{ "", "a hitscan weapon", "a projectile weapon", "melee", "a medigun", "something else" };

	if (m_ctx.player_class <= CLASS_NONE || m_ctx.player_class > CLASS_ENGINEER) {
		return {};
	}

	std::string str{ class_names[m_ctx.player_class] };

	if (m_ctx.weapon != HeldWeapon::NONE) {
		str += std::format(", holding {}", weapon_names[static_cast<int>(m_ctx.weapon)]);
	}

	return str;
}

bool ContextHighlight::section(std::string_view id) const
{
	if (!active()) {
		return false;
	}

	const int cls{ m_ctx.player_class };
	const HeldWeapon weapon{ m_ctx.weapon };

	if (id == "aimbot_melee") {
		return weapon == HeldWeapon::MELEE;
	}

	if (id == "aimbot_hitscan") {
		return weapon == HeldWeapon::HITSCAN;
	}

	if (id == "aimbot_projectile") {
		return weapon == HeldWeapon::PROJECTILE;
	}

	if (id == "auto_backstab") {
		return cls == CLASS_SPY;
	}

	if (id == "auto_detonate") {
		return cls == CLASS_DEMOMAN;
	}

	if (id == "auto_vaccinator") {
		return cls == CLASS_MEDIC;
	}

	if (id == "auto_sentry_wiggler") {
		return cls == CLASS_ENGINEER;
	}

	return false;
}

bool ContextHighlight::var(std::string_view name) const
{
	if (!active()) {
		return false;
	}

	const int cls{ m_ctx.player_class };

	//only settings sitting in a section that isn't class specific, the rest come with their section
	if (name == "aimbot_hitscan_auto_rev_minigun") {
		return cls == CLASS_HEAVY;
	}

	if (name.starts_with("aimbot_projectile_smooth_flamethrowers")) {
		return cls == CLASS_PYRO;
	}

	if (name == "auto_disguise") {
		return cls == CLASS_SPY;
	}

	if (name == "auto_charge_turn") {
		return cls == CLASS_DEMOMAN;
	}

	if (name == "auto_reflect") {
		return cls == CLASS_PYRO;
	}

	if (name == "remove_scope" || name == "fov_override_value_zoomed") {
		return cls == CLASS_SNIPER;
	}

	return false;
}
