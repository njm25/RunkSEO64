#include "menu.hpp"
#include "widgets.hpp"
#include "ui.hpp"

#include "../cfg.hpp"

#include <build_info.hpp>

void Menu::run()
{
	aa_hovered = false;

	if (!ui->isOpen()) {
		return;
	}

	if (ImGui::Begin("Runkseo64", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
	{
		// space reserved at the bottom for the footer
		m_footer_h = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;

		ImGui::SetWindowSize({ 600.0f, 460.0f + m_footer_h }, ImGuiCond_::ImGuiCond_Once);

		if (ImGui::BeginTabBar("tabbar_main"))
		{
			enum class Tabs {
				AIMBOT, AUTOMATION, EXPLOITS, VISUALS, MISC, PLAYERS, CONFIGS
			};

			static Tabs tab{ Tabs::AIMBOT };

			if (ImGui::BeginTabItem("aimbot")) {
				tab = Tabs::AIMBOT;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("automation")) {
				tab = Tabs::AUTOMATION;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("exploits")) {
				tab = Tabs::EXPLOITS;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("visuals")) {
				tab = Tabs::VISUALS;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("misc")) {
				tab = Tabs::MISC;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("players")) {
				tab = Tabs::PLAYERS;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("configs")) {
				tab = Tabs::CONFIGS;
				ImGui::EndTabItem();
			}

			switch (tab)
			{
				case Tabs::AIMBOT: {
					tabAimbot();
					break;
				}

				case Tabs::AUTOMATION: {
					tabAutomation();
					break;
				}

				case Tabs::EXPLOITS: {
					tabExploits();
					break;
				}

				case Tabs::VISUALS: {
					tabVisuals();
					break;
				}

				case Tabs::MISC: {
					tabMisc();
					break;
				}

				case Tabs::PLAYERS: {
					tabPlayers();
					break;
				}

				case Tabs::CONFIGS: {
					tabConfigs();
					break;
				}
			}

			ImGui::EndTabBar();
		}

		// footer
		{
			const std::string build{ std::format("build {} ({})", BUILD_NUMBER, BUILD_HASH) };

			ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - ImGui::GetTextLineHeight());
			ImGui::TextDisabled("Runkseo64");
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
}
