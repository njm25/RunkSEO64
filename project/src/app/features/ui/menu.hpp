#pragma once

#include "../../../game/game.hpp"

class Menu final : public HasUnload
{
public:
	bool aa_hovered{};

private:
	float m_footer_h{};
	float m_aa_hover_time{};
	float m_aa_hover_time_prev{};

private:
	void tabAimbot();
	void pageAimbotMain();
	void pageAimbotVisuals();

	void tabAutomation();
	void tabExploits();

	void tabVisuals();
	void sectionEspSettings();
	void pageVisualsEntities();
	void pageVisualsMap();
	void pageVisualsOther();
	void pageVisualsColors();

	void tabMisc();
	void tabPlayers();
	void tabConfigs();

public:
	void run();
};

MAKE_UNIQUE(Menu, menu);
