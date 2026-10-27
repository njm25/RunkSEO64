#include "menu.hpp"
#include "widgets.hpp"
#include "ui.hpp"
#include "configs.hpp"

#include "../cfg.hpp"

#include <build_info.hpp>

void Menu::navigate(const MenuTab tab, const int sub_tab)
{
	m_tab = tab;

	if (tab == MenuTab::AIMBOT) {
		m_aimbot_tab = static_cast<AimbotTab>(sub_tab);
	}

	else if (tab == MenuTab::VISUALS) {
		m_visuals_tab = static_cast<VisualsTab>(sub_tab);
	}

	m_navigate_frames = 2;
}

void Menu::run()
{
	aa_hovered = false;

	m_aa_hover_time_prev = m_aa_hover_time;
	m_aa_hover_time = 0.0f;

	if (!ui->isOpen()) {
		return;
	}

	if (!m_registered) {
		registerSections();
		m_registered = true;
	}

	if (ImGui::Begin("Runkseo64", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
	{
		// space reserved at the bottom for the footer
		m_footer_h = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;

		const float search_h{ ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y };

		ImGui::SetWindowSize({ scaled(600.0f), scaled(460.0f) + m_footer_h + search_h }, ImGuiCond_::ImGuiCond_Once);

		drawSearchBar();

		if (ImGui::BeginTabBar("tabbar_main"))
		{
			tabItem("aimbot", m_tab, MenuTab::AIMBOT);
			tabItem("automation", m_tab, MenuTab::AUTOMATION);
			tabItem("exploits", m_tab, MenuTab::EXPLOITS);
			tabItem("visuals", m_tab, MenuTab::VISUALS);
			tabItem("misc", m_tab, MenuTab::MISC);
			tabItem("players", m_tab, MenuTab::PLAYERS);
			tabItem("binds", m_tab, MenuTab::BINDS);
			tabItem("configs", m_tab, MenuTab::CONFIGS);

			if (!m_search.empty()) {
				drawSearchResults();
			}

			else
			{
				switch (m_tab)
				{
					case MenuTab::AIMBOT: {
						tabAimbot();
						break;
					}

					case MenuTab::AUTOMATION: {
						tabAutomation();
						break;
					}

					case MenuTab::EXPLOITS: {
						tabExploits();
						break;
					}

					case MenuTab::VISUALS: {
						tabVisuals();
						break;
					}

					case MenuTab::MISC: {
						tabMisc();
						break;
					}

					case MenuTab::PLAYERS: {
						tabPlayers();
						break;
					}

					case MenuTab::BINDS: {
						tabBinds();
						break;
					}

					case MenuTab::CONFIGS: {
						tabConfigs();
						break;
					}
				}
			}

			ImGui::EndTabBar();
		}

		drawConfirm();

		// footer
		{
			const std::string build{ std::format("build {} ({})", BUILD_NUMBER, BUILD_HASH) };

			ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - ImGui::GetTextLineHeight());
			ImGui::TextDisabled("Runkseo64");

			if (!configs->active().empty())
			{
				ImGui::SameLine();
				ImGui::TextDisabled("- %s%s", configs->active().c_str(), configs->hasUnsavedChanges() ? " *" : "");

				if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
					ImGui::SetTooltip(configs->hasUnsavedChanges() ? "loaded config, with unsaved changes" : "loaded config");
				}
			}
			ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - ImGui::CalcTextSize(build.c_str()).x);
			ImGui::TextDisabled("%s", build.c_str());
		}

		if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			ImGui::SetWindowPos({ cfg::ui_pos_menu_x, cfg::ui_pos_menu_y });
		}

		cfg::ui_pos_menu_x = ImGui::GetWindowPos().x;
		cfg::ui_pos_menu_y = ImGui::GetWindowPos().y;

		ImGui::End();
	}

	if (m_navigate_frames > 0) {
		m_navigate_frames--;
	}
}
