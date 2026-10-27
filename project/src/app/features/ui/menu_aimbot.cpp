#include "menu.hpp"
#include "widgets.hpp"

#include "../cfg.hpp"

void Menu::tabAimbot()
{
	if (ImGui::BeginTabBar("tabbar_aimbot"))
	{
		tabItem("main", m_aimbot_tab, AimbotTab::MAIN, "aimbot");
		tabItem("visuals", m_aimbot_tab, AimbotTab::VISUALS, "aimbot");

		if (m_aimbot_tab == AimbotTab::MAIN) {
			pageAimbotMain();
		}

		else if (m_aimbot_tab == AimbotTab::VISUALS) {
			pageAimbotVisuals();
		}

		ImGui::EndTabBar();
	}
}

void Menu::pageAimbotMain()
{
	beginPage("group_aimbot_main", m_footer_h);
	{
		beginColumn("group_aimbot_main0");
		{
			section("aimbot_global", "global", 180.0f, []
			{
				toggle("active", &cfg::aimbot_active);
				toggle("active on attack", &cfg::aimbot_active_on_attack);

				sep();

				toggle("auto shoot", &cfg::aimbot_auto_shoot);
				toggle("strafe prediction", &cfg::aimbot_strafe_pred);

				sep();

				toggle("fakelag fix", &cfg::aimbot_fakelag_fix);

				sep();

				selectMulti("ignore",
							{
								{ "friend", &cfg::aimbot_ignore_friends },
								{ "invisible", &cfg::aimbot_ignore_invisible },
								{ "invulnerable", &cfg::aimbot_ignore_invulnerable },
								{ "sentry", &cfg::aimbot_ignore_sentries },
								{ "dispenser", &cfg::aimbot_ignore_dispensers },
								{ "teleporter", &cfg::aimbot_ignore_teleporters },
								{ "sapped building", &cfg::aimbot_ignore_disabled_buildings },
								{ "sentry buster", &cfg::aimbot_ignore_sentry_busters },
							});
			});

			section("aimbot_melee", "melee", 180.0f, []
			{
				toggle("active", &cfg::aimbot_melee_active);
				toggle("walk to target", &cfg::aimbot_melee_walk_to_target);

				sep();

				selectSingle("mode", &cfg::aimbot_melee_mode, { { "fov", 0 }, { "dist", 1 } });
				selectSingle("aim method", &cfg::aimbot_melee_aim_method, { { "normal", 0 }, { "smooth", 1 }, { "silent", 2 }, { "psilent", 3 } });
				sliderFloat("fov", &cfg::aimbot_melee_fov, 1.0f, 180.0f, "%.0f");
				sliderFloat("smooth", &cfg::aimbot_melee_smooth, 1.0f, 100.0f, "%.0f%%");

				sep();

				toggle("swing prediction", &cfg::aimbot_melee_swing_pred);
				sliderFloat("swing time", &cfg::aimbot_melee_swing_pred_time, 0.1f, 0.2f, "%.2fs");
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_aimbot_main1");
		{
			section("aimbot_hitscan", "hitscan", 180.0f, []
			{
				toggle("active", &cfg::aimbot_hitscan_active);
				toggle("auto rev minigun", &cfg::aimbot_hitscan_auto_rev_minigun);

				sep();

				selectSingle("mode", &cfg::aimbot_hitscan_mode, { { "fov", 0 }, { "distance", 1 } });
				selectSingle("aim method", &cfg::aimbot_hitscan_aim_method, { { "normal", 0 }, { "smooth", 1 }, { "silent", 2 }, { "psilent", 3 } });
				selectSingle("aim position", &cfg::aimbot_hitscan_aim_pos, { { "auto", 0 }, { "body", 1 }, { "head", 2 } });
				sliderFloat("fov", &cfg::aimbot_hitscan_fov, 1.0f, 180.0f, "%.0f");
				sliderFloat("smooth", &cfg::aimbot_hitscan_smooth, 1.0f, 100.0f, "%.0f%%");
				sliderFloat("hitchance", &cfg::aimbot_hitscan_hitchance, 0.0f, 100.0f, "%.0f%%");

				sep();

				toggle("wait for headshot", &cfg::aimbot_hitscan_wait_for_headshot);

				sep();

				toggle("scan hitboxes", &cfg::aimbot_hitscan_scan_hitboxes);
				toggle("scan bounding box", &cfg::aimbot_hitscan_bbox_multipoint);

				sep();

				toggle("target stickies", &cfg::aimbot_hitscan_target_stickies);
				toggle("target rockets (mvm)", &cfg::aimbot_hitscan_target_rockets_minigun_mvm);
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_aimbot_main2");
		{
			section("aimbot_projectile", "projectile", 200.0f, []
			{
				toggle("active", &cfg::aimbot_projectile_active);
				toggle("splash on ground only", &cfg::aimbot_projectile_splash_on_ground_only);

				sep();

				selectSingle("mode", &cfg::aimbot_projectile_mode, { { "fov to current", 0 }, { "fov to predicted", 1 }, { "distance", 2 } });
				selectSingle("aim method", &cfg::aimbot_projectile_aim_method, { { "normal", 0 }, { "snap", 1 }, { "silent", 2 }, { "psilent", 3 } });
				selectSingle("aim position", &cfg::aimbot_projectile_aim_pos, { { "auto", 0 }, { "feet", 1 }, { "body", 2 }, { "head", 3 } });
				sliderFloat("fov", &cfg::aimbot_projectile_fov, 1.0f, 180.0f, "%.0f");

				sep();

				toggle("snap smooth", &cfg::aimbot_projectile_snap_smooth);
				sliderFloat("snap time", &cfg::aimbot_projectile_snap_time, 0.05f, 0.3f, "%.2fs");
				sliderFloat("snap smooth start", &cfg::aimbot_projectile_snap_smooth_start, 1.0f, 100.0f, "%.0f%%");
				sliderFloat("snap smooth end", &cfg::aimbot_projectile_snap_smooth_end, 1.0f, 100.0f, "%.0f%%");

				sep();

				sliderInt("max sim targets", &cfg::aimbot_projectile_max_sim_targets, 1, 6);
				sliderFloat("max sim time", &cfg::aimbot_projectile_max_sim_time, 0.5f, 5.0f, "%.1fs");

				sep();

				toggle("scan bounding box", &cfg::aimbot_projectile_bbox_multipoint);
				sliderInt("multipoint scale", &cfg::aimbot_projectile_multipoint_scale, 50, 100, "%d%%");
				selectSingle("scan splash points", &cfg::aimbot_projectile_splash_multipoint, { { "off", 0 }, { "on", 1 }, { "preferred", 2 } });

				sep();

				toggle("smooth flamethrower", &cfg::aimbot_projectile_smooth_flamethrowers_active);
				sliderFloat("smooth", &cfg::aimbot_projectile_smooth_flamethrowers, 1.0f, 100.0f, "%.0f%%");
			});
		}
		endColumn();
	}
	endPage();
}

void Menu::pageAimbotVisuals()
{
	beginPage("group_aimbot_visuals", m_footer_h);
	{
		beginColumn("group_aimbot_visuals0");
		{
			section("prediction_path", "prediction path", 180.0f, []
			{
				toggle("active", &cfg::prediction_path_active);
				toggle("no depth", &cfg::prediction_path_no_depth);

				sep();

				sliderFloat("duration", &cfg::prediction_path_duration, 1.0f, 10.0f, "%.1fs");

				sep();

				selectSingle("style", &cfg::prediction_path_style, { { "line", 0 }, { "spaced line", 1 }, { "spaced sphere", 2 }, { "arrows", 3 }, {"spiked", 4} });

				sep();

				colorPicker("color", &cfg::prediction_path_color);
			});

			section("fov_circle", "fov circle", 180.0f, []
			{
				toggle("active", &cfg::fov_circle_active);
				toggle("without aimbot", &cfg::fov_circle_without_aimbot);

				sep();

				selectSingle("style", &cfg::fov_circle_style, { { "static", 0 }, { "spinning spaced", 1 } });

				sep();

				sliderInt("spacing", &cfg::fov_circle_spacing, 2, 10);
				sliderFloat("spin speed", &cfg::fov_circle_spin_speed, 0.1f, 1.0f, "%.2f");

				sep();

				colorPicker("color", &cfg::fov_circle_color);
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_aimbot_visuals1");
		{
			section("hitboxes", "hitboxes", 200.0f, []
			{
				toggle("active", &cfg::hitboxes_active);
				toggle("no depth", &cfg::hitboxes_no_depth);
				toggle("clear on shot", &cfg::hitboxes_clear_on_shot);

				sep();

				sliderFloat("duration", &cfg::hitboxes_duration, 0.5f, 5.0f, "%.1fs");

				sep();

				selectSingle("style", &cfg::hitboxes_style, { { "box", 0 }, { "filled box", 1 }, { "both", 2 } });
				selectSingle("timeout style", &cfg::hitboxes_timeout_style, { { "instant", 0 }, { "fade out", 1 } });
				sliderFloat("fade out duration", &cfg::hitboxes_fade_out_duration, 0.1f, 1.0f, "%.1fs");

				sep();

				toggle("show target hitbox", &cfg::hitboxes_show_target_hitbox);

				sep();

				colorPicker("color", &cfg::hitboxes_color);
				colorPicker("target hitbox color", &cfg::hitboxes_target_hitbox_color);
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_aimbot_visuals2");
		{
			section("lag_comp_visuals", "lag compensation", 180.0f, []
			{
				toggle("active", &cfg::lag_comp_visuals_active);
				toggle("last only", &cfg::lag_comp_visuals_last_only);
				toggle("no depth", &cfg::lag_comp_visuals_no_depth);

				sep();

				selectSingle("material", &cfg::lag_comp_visuals_mat, { { "shaded", 0 }, { "flat", 1 }, { "outlined", 2 }, { "hitboxes", 3 } });

				sep();

				colorPicker("color", &cfg::lag_comp_visuals_clr);
			});
		}
		endColumn();
	}
	endPage();
}
