#include "menu.hpp"
#include "widgets.hpp"
#include "ui.hpp"
#include "ui_sound.hpp"
#include "menu_context.hpp"
#include "connection.hpp"

#include "../diagnostics/diagnostics.hpp"

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

	else if (tab == MenuTab::SKINS) {
		m_skins_tab = static_cast<SkinsTab>(sub_tab);
	}

	m_navigate_frames = 2;
}

static float easeOut(const float t)
{
	return 1.0f - std::pow(1.0f - std::clamp(t, 0.0f, 1.0f), 3.0f);
}

//moves towards the target over `duration` seconds, or jumps there with animations off
static float animate(const float cur, const float target, const float duration)
{
	if (!cfg::ui_animations) {
		return target;
	}

	const float step{ ImGui::GetIO().DeltaTime / duration };

	return cur < target ? std::min(cur + step, target) : std::max(cur - step, target);
}

void Menu::beginSubTabPage()
{
	const float e{ easeOut(m_sub_tab_anim) };

	//on top of the main tab's fade, which is already in the style alpha
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * e);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (1.0f - e) * scaled(6.0f));
}

void Menu::endSubTabPage()
{
	ImGui::PopStyleVar();
}

void Menu::run()
{
	aa_hovered = false;

	m_aa_hover_time_prev = m_aa_hover_time;
	m_aa_hover_time = 0.0f;

	const bool open{ ui->isOpen() };

	//keeps drawing while it fades out after being closed
	m_open_anim = animate(m_open_anim, open ? 1.0f : 0.0f, 0.15f);

	if (m_open_anim <= 0.0f) {
		m_tab_key = -1;
		return;
	}

	if (!m_registered) {
		registerSections();
		m_registered = true;
	}

	context_highlight->update(cfg::ui_context_highlight && open ? currentMenuContext() : MenuContext{});

	if (cfg::ui_connection_info && open) {
		connection->update(static_cast<float>(ImGui::GetTime()));
	}

	//loaded into an updated game and things broke, show what
	if (open && diagnostics->consumeOpenRequest()) {
		navigate(MenuTab::DIAGNOSTICS);
	}

	const ImVec2 display{ ImGui::GetIO().DisplaySize };

	//the saved size wins unless the window is being resized
	if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		ImGui::SetNextWindowSize({ scaled(cfg::ui_menu_w), scaled(cfg::ui_menu_h) });
	}

	ImGui::SetNextWindowSizeConstraints({ scaled(480.0f), scaled(320.0f) }, { std::max(display.x, scaled(480.0f)), std::max(display.y, scaled(320.0f)) });

	//fade out while the mouse is somewhere else, but not while a dropdown or popup from the menu is in use
	const bool in_use{ m_hovered || ImGui::IsAnyItemActive() || ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) };

	m_fade = animate(m_fade, cfg::ui_fade_unhovered && !in_use ? std::clamp(cfg::ui_fade_opacity, 0.1f, 1.0f) : 1.0f, 0.2f);

	//eases in when opening, drops off quickly when closing
	const float open_e{ open ? easeOut(m_open_anim) : m_open_anim * m_open_anim };

	//slides up a little as it opens
	const float slide{ (1.0f - open_e) * scaled(12.0f) };

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * open_e * m_fade);

	ImGuiWindowFlags window_flags{ ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar };

	//closing, just fading out
	if (!open) {
		window_flags |= ImGuiWindowFlags_NoInputs;
	}

	//gradient and image backgrounds are drawn by the ui instead of the window
	const bool custom_bg{ ui->customBackground() };

	if (custom_bg) {
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
	}

	const bool visible{ ImGui::Begin("RunkSEO64", nullptr, window_flags) };

	if (custom_bg) {
		ImGui::PopStyleColor();
	}

	if (visible)
	{
		if (custom_bg) {
			ui->drawBackground(ImGui::GetWindowDrawList(), ImGui::GetWindowPos(), ImGui::GetWindowSize());
		}

		m_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

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
			tabItem("skins", m_tab, MenuTab::SKINS);
			tabItem("players", m_tab, MenuTab::PLAYERS);
			tabItem("binds", m_tab, MenuTab::BINDS);
			tabItem("statistics", m_tab, MenuTab::STATISTICS);
			tabItem("configs", m_tab, MenuTab::CONFIGS);

			const bool diag_failed{ diagnostics->failedAtLoad() > 0 };

			if (diag_failed) {
				ImGui::PushStyleColor(ImGuiCol_Text, warningColor());
			}

			tabItem("diagnostics", m_tab, MenuTab::DIAGNOSTICS);

			if (diag_failed) {
				ImGui::PopStyleColor();
			}

			//fade and slide the whole tab in when the main tab changes
			const int tab_key{ m_search.empty() ? static_cast<int>(m_tab) : -2 };
			const bool tab_changed{ tab_key != m_tab_key };

			if (tab_changed) {
				m_tab_anim = m_tab_key == -1 ? 1.0f : 0.0f; //not on the first frame, the open animation covers that
				m_tab_key = tab_key;
			}

			//a sub tab change only animates the page under its bar, the main tab animation already covers a main tab change
			const int sub_tab_key{ static_cast<int>(m_skins_tab) * 1000 + static_cast<int>(m_stats_tab) * 100 + static_cast<int>(m_aimbot_tab) * 10 + static_cast<int>(m_visuals_tab) };

			if (sub_tab_key != m_sub_tab_key) {
				m_sub_tab_anim = tab_changed || m_sub_tab_key == -1 ? 1.0f : 0.0f;
				m_sub_tab_key = sub_tab_key;
			}

			m_tab_anim = animate(m_tab_anim, 1.0f, 0.15f);
			m_sub_tab_anim = animate(m_sub_tab_anim, 1.0f, 0.15f);

			const float tab_e{ easeOut(m_tab_anim) };

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * tab_e);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (1.0f - tab_e) * scaled(6.0f));

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

					case MenuTab::SKINS: {
						tabSkins();
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

					case MenuTab::STATISTICS: {
						tabStatistics();
						break;
					}

					case MenuTab::CONFIGS: {
						tabConfigs();
						break;
					}

					case MenuTab::DIAGNOSTICS: {
						tabDiagnostics();
						break;
					}
				}
			}

			ImGui::PopStyleVar();

			ImGui::EndTabBar();
		}

		drawConfirm();

		//any click on something clickable, popups and dropdowns included
		if (cfg::ui_sounds && open && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsAnyItemHovered()) {
			playClickSound(cfg::ui_sound_volume);
		}

		// footer
		{
			const std::string build{ std::format("build {} ({})", BUILD_NUMBER, BUILD_HASH) };

			ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - ImGui::GetTextLineHeight());
			ImGui::TextDisabled("RunkSEO64 -");
			ImGui::SameLine();
			drawConfigSwitcher();

			const float build_x{ ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - ImGui::CalcTextSize(build.c_str()).x };

			if (cfg::ui_connection_info) {
				connection->drawFooter(build_x - ImGui::GetStyle().ItemSpacing.x * 3.0f);
			}

			ImGui::SameLine(build_x);
			ImGui::TextDisabled("%s", build.c_str());
		}

		if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			keepOnScreen(cfg::ui_pos_menu_x, cfg::ui_pos_menu_y, ImGui::GetWindowSize());
			ImGui::SetWindowPos({ cfg::ui_pos_menu_x, cfg::ui_pos_menu_y + slide });
		}

		//only taken from the window while dragging, so a reset or loaded config isn't overwritten,
		//and the open slide (which imgui rounds to whole pixels) never creeps into the saved position
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			cfg::ui_pos_menu_x = ImGui::GetWindowPos().x;
			cfg::ui_pos_menu_y = ImGui::GetWindowPos().y - slide;

			cfg::ui_menu_w = ImGui::GetWindowWidth() / ui->scale();
			cfg::ui_menu_h = ImGui::GetWindowHeight() / ui->scale();
		}
	}

	ImGui::End();

	ImGui::PopStyleVar();

	if (m_navigate_frames > 0) {
		m_navigate_frames--;
	}
}
