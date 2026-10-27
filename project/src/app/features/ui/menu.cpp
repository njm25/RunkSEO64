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

	const ImVec2 display{ ImGui::GetIO().DisplaySize };

	ImGui::SetNextWindowSizeConstraints({ scaled(480.0f), scaled(320.0f) }, { std::max(display.x, scaled(480.0f)), std::max(display.y, scaled(320.0f)) });

	//same as the position below, the saved size wins unless the window is being resized
	if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		ImGui::SetNextWindowSize({ scaled(cfg::ui_menu_w), scaled(cfg::ui_menu_h) });
	}

	if (ImGui::Begin("Runkseo64", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
	{
		// space reserved at the bottom for the footer
		m_footer_h = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;

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

		cfg::ui_menu_w = ImGui::GetWindowWidth() / ui->scale();
		cfg::ui_menu_h = ImGui::GetWindowHeight() / ui->scale();
	}

	ImGui::End();

	if (m_navigate_frames > 0) {
		m_navigate_frames--;
	}
}
