#include "menu.hpp"
#include "widgets.hpp"

#include "../skin_changer/skin_changer.hpp"
#include "../cfg.hpp"

//paint kit picker for one slot, the list comes from whatever weapon is equipped there right now
static void skinCombo(const char *const label, const int slot, int *const var)
{
	if (!customWidget(label, var)) {
		return;
	}

	const SkinSlot info{ skinSlot(slot) };

	std::string preview{ "none" };

	for (const SkinOption &kit : info.kits)
	{
		if (kit.id == *var) {
			preview = kit.name;
		}
	}

	//picked for a weapon that isn't equipped right now
	if (*var && preview == "none") {
		preview = std::format("paint kit {}", *var);
	}

	if (ImGui::BeginCombo(label, preview.c_str()))
	{
		if (ImGui::Selectable("none", *var == 0)) {
			*var = 0;
		}

		if (info.weapon.empty()) {
			ImGui::TextDisabled("join a game to see your weapon's paints");
		}

		else if (info.kits.empty()) {
			ImGui::TextDisabled("%s can't take war paints", info.weapon.c_str());
		}

		for (const SkinOption &kit : info.kits)
		{
			ImGui::PushID(kit.id);

			if (ImGui::Selectable(kit.name.c_str(), kit.id == *var)) {
				*var = kit.id;
			}

			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	settingContext(label, var);

	if (!info.weapon.empty()) {
		ImGui::TextDisabled("%s", info.weapon.c_str());
	}
}

void Menu::pageVisualsAwareness()
{
	beginPage("group_visuals_awareness", m_footer_h);
	{
		beginColumn("group_visuals_awareness0");
		{
			section("callouts", "item callouts", 180.0f, []
			{
				toggle("active", &cfg::callouts_active);
				colorPicker("color", &cfg::callouts_color);
			});

			section("respawn_timers", "respawn timers", 180.0f, []
			{
				toggle("active", &cfg::respawn_timers_active);
				toggle("teammates", &cfg::respawn_timers_teammates);
				sliderFloat("position", &cfg::respawn_timers_pos, 0.0f, 1.0f, "%.2f");
			});

			section("pack_timers", "pack timers", 180.0f, []
			{
				toggle("active", &cfg::pack_timers_active);
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_visuals_awareness1");
		{
			section("sentry_ranges", "sentry ranges", 180.0f, []
			{
				toggle("active", &cfg::sentry_ranges_active);
				toggle("only when near", &cfg::sentry_ranges_only_near);
				toggle("warn when seen", &cfg::sentry_warning);
			});

			section("sticky_zones", "sticky zones", 180.0f, []
			{
				toggle("active", &cfg::sticky_zones_active);
				toggle("warn when in range", &cfg::sticky_warning);
			});

			section("sniper_warning", "snipers", 180.0f, []
			{
				toggle("warn when aimed at", &cfg::sniper_warning);
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_visuals_awareness2");
		{
			section("freecam", "freecam", 180.0f, []
			{
				toggle("while dead", &cfg::freecam_active);
				sliderFloat("speed", &cfg::freecam_speed, 100.0f, 2000.0f, "%.0f");
			});

			section("skin_changer", "skin changer", 200.0f, []
			{
				toggle("active", &cfg::skins_active);
				selectSingle("wear", &cfg::skins_wear, { { "factory new", 0 }, { "minimal wear", 1 }, { "field tested", 2 }, { "well worn", 3 }, { "battle scarred", 4 } });

				sep();

				skinCombo("primary", 0, &cfg::skins_primary);
				skinCombo("secondary", 1, &cfg::skins_secondary);
				skinCombo("melee", 2, &cfg::skins_melee);
			});
		}
		endColumn();
	}
	endPage();
}
