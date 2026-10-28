#pragma once

#include "../../../game/game.hpp"

//fly around freely while dead instead of watching the death and spectator cams
class Freecam final : public HasLevelInit
{
private:
	bool m_flying{};
	vec3 m_pos{};

public:
	bool onLevelInit() override;

public:
	//true when it moved the view, called from OverrideView after the game's own camera is set up
	bool overrideView(CViewSetup *const setup);
	bool flying() const;
};

MAKE_UNIQUE(Freecam, freecam);
