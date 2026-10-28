#include "menu.hpp"
#include "widgets.hpp"

#include "../cfg.hpp"

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
	}
	endPage();
}
