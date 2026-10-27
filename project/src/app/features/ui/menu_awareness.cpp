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

//everything the skin changer can put on one weapon slot
static void skinSlotSection(const char *const id, const char *const title, const int slot, int *const kit, int *const unusual, int *const sheen,
							bool *const australium, bool *const festive, int *const style)
{
	section(id, title, 200.0f, [=]
	{
		skinCombo("war paint", slot, kit);

		sep();

		selectSingle("unusual", unusual, { { "none", 0 }, { "hot", 1 }, { "isotope", 2 }, { "cool", 3 }, { "energy orb", 4 } });

		selectSingle("sheen", sheen,
		{
			{ "none", 0 }, { "team shine", 1 }, { "deadly daffodil", 2 }, { "manndarin", 3 },
			{ "mean green", 4 }, { "agonizing emerald", 5 }, { "villainous violet", 6 }, { "hot rod", 7 }
		});

		toggle("australium", australium);
		toggle("festivized", festive);

		selectSingle("style", style, { { "default", 0 }, { "style 1", 1 }, { "style 2", 2 }, { "style 3", 3 }, { "style 4", 4 } });
	});
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

			section("freecam", "freecam", 180.0f, []
			{
				toggle("while dead", &cfg::freecam_active);
				sliderFloat("speed", &cfg::freecam_speed, 100.0f, 2000.0f, "%.0f");
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_visuals_awareness2");
		{
			section("skin_changer", "skin changer", 200.0f, []
			{
				toggle("active", &cfg::skins_active);
				selectSingle("wear", &cfg::skins_wear, { { "factory new", 0 }, { "minimal wear", 1 }, { "field tested", 2 }, { "well worn", 3 }, { "battle scarred", 4 } });
			});

			skinSlotSection("skins_primary", "primary skin", 0, &cfg::skins_primary, &cfg::skins_primary_unusual, &cfg::skins_primary_sheen,
							&cfg::skins_primary_australium, &cfg::skins_primary_festive, &cfg::skins_primary_style);

			skinSlotSection("skins_secondary", "secondary skin", 1, &cfg::skins_secondary, &cfg::skins_secondary_unusual, &cfg::skins_secondary_sheen,
							&cfg::skins_secondary_australium, &cfg::skins_secondary_festive, &cfg::skins_secondary_style);

			skinSlotSection("skins_melee", "melee skin", 2, &cfg::skins_melee, &cfg::skins_melee_unusual, &cfg::skins_melee_sheen,
							&cfg::skins_melee_australium, &cfg::skins_melee_festive, &cfg::skins_melee_style);
		}
		endColumn();
	}
	endPage();
}
