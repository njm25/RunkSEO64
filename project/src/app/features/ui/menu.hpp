#pragma once

#include "../../../game/game.hpp"

enum class MenuTab
{
	AIMBOT, AUTOMATION, EXPLOITS, VISUALS, MISC, PLAYERS, BINDS, CONFIGS
};

enum class AimbotTab
{
	MAIN, VISUALS
};

enum class VisualsTab
{
	ENTITIES, MAP, OTHER, COLORS
};

class Menu final : public HasUnload
{
public:
	bool aa_hovered{};

private:
	float m_footer_h{};
	float m_aa_hover_time{};
	float m_aa_hover_time_prev{};

private:
	MenuTab m_tab{ MenuTab::AIMBOT };
	AimbotTab m_aimbot_tab{ AimbotTab::MAIN };
	VisualsTab m_visuals_tab{ VisualsTab::ENTITIES };
	int m_navigate_frames{}; //tab bars apply a forced selection a frame late, so it's held for a couple of frames

private:
	float m_open_anim{}; //0 closed, 1 fully open
	float m_fade{ 1.0f }; //fade while the mouse is elsewhere
	bool m_hovered{};
	float m_tab_anim{ 1.0f };
	int m_tab_key{ -1 }; //what's being shown, a change restarts the tab animation

private:
	std::string m_search{};
	bool m_registered{};

private:
	template <typename T>
	void tabItem(const char *const label, T &cur, const T value);

public:
	void navigate(const MenuTab tab, const int sub_tab = 0);

private:
	struct SearchPage;
	static const std::vector<SearchPage> &searchPages();

	void registerSections();
	void drawSearchBar();
	void drawSearchResults();

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
	void tabBinds();
	void tabConfigs();

public:
	void run();
};

MAKE_UNIQUE(Menu, menu);

//only used by the tab files
template <typename T>
void Menu::tabItem(const char *const label, T &cur, const T value)
{
	const bool navigating{ m_navigate_frames > 0 };

	if (ImGui::BeginTabItem(label, nullptr, navigating && cur == value ? ImGuiTabItemFlags_SetSelected : 0))
	{
		if (!navigating) {
			cur = value;
		}

		ImGui::EndTabItem();
	}

	//picking a tab leaves the search results
	if (ImGui::IsItemClicked()) {
		m_search.clear();
	}
}
