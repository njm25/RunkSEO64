#pragma once

#include "../../../game/game.hpp"

#include "widgets.hpp"

enum class MenuTab
{
	AIMBOT, AUTOMATION, EXPLOITS, VISUALS, MISC, SKINS, PLAYERS, BINDS, STATISTICS, CONFIGS, DIAGNOSTICS
};

enum class AimbotTab
{
	MAIN, VISUALS
};

enum class VisualsTab
{
	ENTITIES, MAP, OTHER, COLORS, MENU, AWARENESS
};

enum class StatsTab
{
	OVERVIEW, CLASSES, MAPS, WEAPONS
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
	StatsTab m_stats_tab{ StatsTab::OVERVIEW };
	int m_navigate_frames{}; //tab bars apply a forced selection a frame late, so it's held for a couple of frames

private:
	float m_open_anim{}; //0 closed, 1 fully open
	float m_fade{ 1.0f }; //fade while the mouse is elsewhere
	bool m_hovered{};
	float m_tab_anim{ 1.0f };
	int m_tab_key{ -1 }; //what's being shown, a change restarts the tab animation
	float m_sub_tab_anim{ 1.0f }; //same for the page under a sub tab bar, the bar itself stays put
	int m_sub_tab_key{ -1 };

private:
	std::string m_search{};
	bool m_registered{};

private:
	//parent is the main tab a sub tab sits under, for context highlighting
	template <typename T>
	void tabItem(const char *const label, T &cur, const T value, const char *const parent = nullptr);

	//around the page a sub tab bar shows, so switching sub tabs only animates the page
	void beginSubTabPage();
	void endSubTabPage();

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
	void pageVisualsMenu();
	void pageVisualsAwareness();

	void tabMisc();
	void tabSkins();
	void tabPlayers();
	void tabBinds();
	void tabStatistics();
	void tabConfigs();
	void tabDiagnostics();
	void requestLoadConfig(const std::string &name); //asks first when there are unsaved changes
	void drawConfigSwitcher();

public:
	void run();
};

MAKE_UNIQUE(Menu, menu);

//only used by the tab files
template <typename T>
void Menu::tabItem(const char *const label, T &cur, const T value, const char *const parent)
{
	const bool navigating{ m_navigate_frames > 0 };
	const bool highlighted{ pathHighlighted(parent ? std::format("{} > {}", parent, label) : std::string{ label }) };

	if (highlighted) {
		ImGui::PushStyleColor(ImGuiCol_Text, highlightColor());
	}

	const bool open{ ImGui::BeginTabItem(label, nullptr, navigating && cur == value ? ImGuiTabItemFlags_SetSelected : 0) };

	if (highlighted) {
		ImGui::PopStyleColor();
	}

	if (open)
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
