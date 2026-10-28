#pragma once

#include "../../../game/game.hpp"

enum class HeldWeapon
{
	NONE, HITSCAN, PROJECTILE, MELEE, MEDIGUN, OTHER
};

struct MenuContext final
{
	bool in_game{};
	int player_class{}; //0 none, then scout to engineer like m_iClass
	HeldWeapon weapon{};
};

//game side, in game_info.cpp
MenuContext currentMenuContext();

//picks out the sections and settings that matter for what you're playing right now, the menu only highlights them
class ContextHighlight final
{
private:
	MenuContext m_ctx{};

public:
	void update(const MenuContext &ctx);

	bool active() const;
	const MenuContext &context() const;
	std::string describe() const; //"spy, holding melee"

	bool section(std::string_view id) const;
	bool var(std::string_view name) const;
};

MAKE_UNIQUE(ContextHighlight, context_highlight);
