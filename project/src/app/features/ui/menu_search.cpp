#include "menu.hpp"
#include "widgets.hpp"
#include "ui.hpp"

struct Menu::SearchPage final
{
	const char *path{};
	MenuTab tab{};
	int sub_tab{};
	void (Menu:: *draw)() {};
};

//every page built from sections, the entities page only contributes its esp settings
const std::vector<Menu::SearchPage> &Menu::searchPages()
{
	static const std::vector<SearchPage> pages
	{
		{ "aimbot > main", MenuTab::AIMBOT, static_cast<int>(AimbotTab::MAIN), &Menu::pageAimbotMain },
		{ "aimbot > visuals", MenuTab::AIMBOT, static_cast<int>(AimbotTab::VISUALS), &Menu::pageAimbotVisuals },
		{ "automation", MenuTab::AUTOMATION, 0, &Menu::tabAutomation },
		{ "exploits", MenuTab::EXPLOITS, 0, &Menu::tabExploits },
		{ "visuals > entities", MenuTab::VISUALS, static_cast<int>(VisualsTab::ENTITIES), &Menu::sectionEspSettings },
		{ "visuals > map", MenuTab::VISUALS, static_cast<int>(VisualsTab::MAP), &Menu::pageVisualsMap },
		{ "visuals > other", MenuTab::VISUALS, static_cast<int>(VisualsTab::OTHER), &Menu::pageVisualsOther },
		{ "visuals > awareness", MenuTab::VISUALS, static_cast<int>(VisualsTab::AWARENESS), &Menu::pageVisualsAwareness },
		{ "visuals > colors", MenuTab::VISUALS, static_cast<int>(VisualsTab::COLORS), &Menu::pageVisualsColors },
		{ "visuals > menu", MenuTab::VISUALS, static_cast<int>(VisualsTab::MENU), &Menu::pageVisualsMenu },
		{ "misc", MenuTab::MISC, 0, &Menu::tabMisc },
		{ "skins", MenuTab::SKINS, 0, &Menu::tabSkins },
	};

	return pages;
}

void Menu::registerSections()
{
	setWidgetMode(WidgetMode::REGISTER);

	for (const SearchPage &page : searchPages()) {
		setSearchPath(page.path);
		(this->*page.draw)();
	}

	setSearchPath("");
	setWidgetMode(WidgetMode::DRAW);
}

void Menu::drawSearchBar()
{
	const bool has_text{ !m_search.empty() };
	const float clear_w{ has_text ? ImGui::CalcTextSize("clear").x + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetStyle().ItemSpacing.x : 0.0f };

	const bool focus{ ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_F) };

	//a box in the top right corner, narrower when the menu is too
	const float avail{ ImGui::GetContentRegionAvail().x };
	const float box_w{ std::min(scaled(220.0f), avail - clear_w) };
	const float box_x{ ImGui::GetCursorPosX() + avail - box_w };

	//clear sits to the left so the box doesn't move while typing
	if (has_text)
	{
		ImGui::SetCursorPosX(box_x - clear_w);

		if (ImGui::Button("clear")) {
			m_search.clear();
		}

		ImGui::SameLine();
	}

	ImGui::SetCursorPosX(box_x);

	if (focus) {
		ImGui::SetKeyboardFocusHere();
	}

	ImGui::SetNextItemWidth(box_w);
	inputStringWithHint("##search", &m_search, "search settings (ctrl+f)");
}

void Menu::drawSearchResults()
{
	ImGui::BeginChild("search_results", { 0.0f, -m_footer_h });
	{
		setWidgetMode(WidgetMode::SEARCH);
		setSearchQuery(m_search);

		int results{};

		for (const SearchPage &page : searchPages())
		{
			(this->*page.draw)();

			results += searchResultCount();

			if (searchGotoClicked()) {
				navigate(page.tab, page.sub_tab);
				m_search.clear();
			}

			//reset the per page counters
			setWidgetMode(WidgetMode::SEARCH);
		}

		setWidgetMode(WidgetMode::DRAW);

		if (!results) {
			ImGui::TextDisabled("no settings match '%s'", m_search.c_str());
		}
	}
	ImGui::EndChild();
}
