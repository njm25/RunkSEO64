#include "menu.hpp"
#include "widgets.hpp"

#include "../cfg.hpp"

void Menu::tabMisc()
{
	beginPage("group_misc", m_footer_h);
	{
		beginColumn("group_misc0");
		{
			section("misc", "misc", 180.0f, []
			{
				toggle("bhop", &cfg::misc_bhop);
				toggle("fast stop", &cfg::misc_faststop);
				toggle("sv_pure bypass", &cfg::misc_sv_pure_bypass);
				toggle("noisemaker spam", &cfg::misc_noisemaker_spam);
				toggle("mvm instant respawn", &cfg::misc_mvm_instant_respawn);
				toggle("rainbow party", &cfg::misc_rainbow_party);
				toggle("giant weapon sounds", &cfg::misc_giant_weapon_sounds);
				toggle("expanded inventory", &cfg::misc_expanded_inventory);
				toggle("bypass f2p chat", &cfg::misc_bypass_f2p_chat);

				sep();

				toggle("medieval chat", &cfg::misc_medieval_chat);
				selectSingle("owo chat", &cfg::misc_owo_chat, { { "off", 0 }, { "owo", 1 }, { "uwu", 2 }, {"uvu", 3 } });
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_misc1");
		{
			beginColumn("group_misc2");
			{
				section("taunt_slide", "taunt slide", 180.0f, []
				{
					toggle("active", &cfg::misc_taunts_slide);
					toggle("control", &cfg::misc_taunts_control);
					toggle("spin", &cfg::misc_taunts_spin);
					toggle("follow camera", &cfg::misc_taunts_follow_camera);

					sep();

					sliderFloat("spin speed", &cfg::misc_taunts_spin_speed, -20.0f, 20.0f, "%.1f");
					selectSingle("spin mode", &cfg::misc_taunts_spin_mode, { { "normal", 0 }, { "random", 1 }, { "sine", 2 }, { "triangle", 3 } });
				});

				section("damage_feedback", "damage feedback", 180.0f, []
				{
					toggle("active", &cfg::misc_damage_feedback_active);

					sep();

					toggle("show invisible spies", &cfg::misc_damage_feedback_show_invisible_spies);
					toggle("show through walls", &cfg::misc_damage_feedback_show_through_walls);
				});
			}
			endColumn();

			nextColumn();

			beginColumn("group_misc3");
			{
				section("removals", "removals", 180.0f, []
				{
					toggle("remove end of match surveys", &cfg::misc_remove_end_of_match_surveys);
					toggle("remove autojoin countdown", &cfg::misc_remove_autojoin_countdown);
					toggle("remove equip regions", &cfg::misc_remove_equip_regions);
					toggle("remove teammate pushaway", &cfg::misc_remove_teammate_pushaway);
					toggle("remove item found notification", &cfg::misc_remove_item_found_notification);
					toggle("remove fix angle", &cfg::misc_remove_fix_angle);
				});

				section("region_selector", "region selector", 180.0f, []
				{
					toggle("active", &cfg::region_selector_active);

					sep();

					toggle("disable europe", &cfg::region_selector_disable_europe);
					toggle("disable na", &cfg::region_selector_disable_na);
					toggle("disable south america", &cfg::region_selector_disable_sa);
					toggle("disable asia", &cfg::region_selector_disable_asia);
					toggle("disable australia", &cfg::region_selector_disable_australia);
					toggle("disable africa", &cfg::region_selector_disable_africa);
				});
			}
			endColumn();
		}
		endColumn();
	}
	endPage();
}
