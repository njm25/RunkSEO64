#include "menu.hpp"
#include "widgets.hpp"

#include "../cfg.hpp"

void Menu::tabAutomation()
{
	beginPage("group_automation", m_footer_h);
	{
		beginColumn("group_automation0");
		{
			section("auto_backstab", "auto backstab", 180.0f, []
			{
				toggle("active", &cfg::auto_backstab_active);
				toggle("legit mode", &cfg::auto_backstab_legit_mode);
				toggle("butterknife if lethal", &cfg::auto_backstab_lethal_active);

				sep();

				selectMulti("ignore",
							{
								{ "friend", &cfg::auto_backstab_ignore_friends },
								{ "invisible", &cfg::auto_backstab_ignore_invisible },
								{ "invulnerable", &cfg::auto_backstab_ignore_invulnerable }
							});
			});

			section("auto_detonate", "auto detonate", 180.0f, []
			{
				toggle("active", &cfg::auto_detonate_active);

				sep();

				selectMulti("ignore",
							{
								{ "friend", &cfg::auto_detonate_ignore_friends },
								{ "invisible", &cfg::auto_detonate_ignore_invisible },
								{ "invulnerable", &cfg::auto_detonate_ignore_invulnerable },
								{ "sentries", &cfg::auto_detonate_ignore_sentries },
								{ "dispensers", &cfg::auto_detonate_ignore_dispensers },
								{ "teleporters", &cfg::auto_detonate_ignore_teleporters },
							});
			});

			section("auto_queue", "auto queue", 180.0f, []
			{
				toggle("active", &cfg::auto_queue_active);
				selectSingle("mode", &cfg::auto_queue_mode, { { "only in menu", 0 }, { "always", 1 } });
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_automation1");
		{
			section("auto_vaccinator", "auto vaccinator", 180.0f, []
			{
				toggle("active", &cfg::auto_vacc_active);
				toggle("prioritise self", &cfg::auto_vacc_prioritise_local);
			});

			section("auto_strafe", "auto strafe", 180.0f, []
			{
				toggle("active", &cfg::auto_strafe_active);

				sep();

				sliderFloat("smoothness", &cfg::auto_strafe_smooth, 0.0f, 1.0f, "%.2f");

				sep();

				selectSingle("mode", &cfg::auto_strafe_mode, { { "on WASD", 0 }, { "always", 1 } });
			});

			section("auto_accept", "auto accept", 180.0f, []
			{
				toggle("active", &cfg::auto_accept_active);
				selectSingle("mode", &cfg::auto_accept_mode, { { "only in menu", 0 }, { "always", 1 } });
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_automation2");
		{
			section("auto_sentry_wiggler", "auto sentry wiggler", 180.0f, []
			{
				toggle("active", &cfg::auto_sentry_wiggler);
				toggle("self rodeo", &cfg::auto_sentry_wiggler_self_rodeo);

				sep();

				sliderFloat("distance", &cfg::auto_sentry_wiggler_distance, 0.03125f, 0.5f, "%.2f");
			});

			section("auto_other", "other", 180.0f, []
			{
				toggle("auto reflect", &cfg::auto_reflect);
				toggle("auto disguise", &cfg::auto_disguise);
				toggle("auto edge jump", &cfg::auto_edge_jump_active);
				toggle("auto charge turn", &cfg::auto_charge_turn);
				toggle("auto mvm ready up", &cfg::auto_mvm_ready_up);
			});
		}
		endColumn();
	}
	endPage();
}
