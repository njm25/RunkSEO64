#include "tooltips.hpp"

#include <unordered_map>

//keyed by cfg var name so every widget bound to the var shares the same description
static const std::unordered_map<std::string_view, const char *> tooltips
{
	{ "aimbot_active_on_attack", "only aim while the attack button is held" },
	{ "aimbot_fakelag_fix", "also target lag records that were interpolated from choked packets" },
	{ "aimbot_hitscan_wait_for_headshot", "hold fire on weapons that can headshot until the shot can headshot" },
	{ "aimbot_hitscan_scan_hitboxes", "scan the other hitboxes when the chosen one can't be hit" },
	{ "aimbot_hitscan_bbox_multipoint", "scan points across the target's bounding box instead of only hitbox centers" },
	{ "aimbot_hitscan_hitchance", "minimum chance for a shot to land before firing, 0 to disable" },
	{ "aimbot_melee_swing_pred_time", "how far ahead to predict when a melee swing will connect" },
	{ "aimbot_projectile_splash_on_ground_only", "only aim splash at targets standing on the ground" },
	{ "aimbot_projectile_max_sim_targets", "how many targets to try simulating per tick, close targets are always tried" },
	{ "aimbot_projectile_max_sim_time", "longest projectile flight time that will be simulated" },
	{ "aimbot_projectile_snap_time", "how long the snap aim method takes to reach the target" },
	{ "aimbot_projectile_bbox_multipoint", "scan points across the target's bounding box" },
	{ "aimbot_projectile_multipoint_scale", "how far towards the edge of the bounding box points are scanned" },
	{ "aimbot_projectile_splash_multipoint", "scan the ground and walls near the target for splash damage" },

	{ "fov_circle_without_aimbot", "draw the circle even when the matching aimbot is off" },
	{ "prediction_path_no_depth", "draw through walls" },
	{ "hitboxes_no_depth", "draw through walls" },
	{ "hitboxes_clear_on_shot", "remove old hitboxes every time you shoot" },
	{ "lag_comp_visuals_no_depth", "draw through walls" },
	{ "lag_comp_visuals_last_only", "only draw the last lag record instead of every record" },

	{ "auto_backstab_legit_mode", "never adjust your aim, only stab when you are already aiming at a valid backstab" },
	{ "auto_backstab_lethal_active", "also stab from any angle when the hit would be lethal" },
	{ "auto_queue_mode", "only in menu stops queueing while you're in a game" },
	{ "auto_sentry_wiggler_self_rodeo", "also wiggle around your own sentry" },
	{ "auto_sentry_wiggler_distance", "distance kept from the sentry, as a fraction of its size" },

	{ "callouts_active", "under each enemy, name the items that change how you fight them: dead ringer, kritzkrieg, vaccinator, gunslinger, shields and so on" },
	{ "respawn_timers_active", "list dead players with how long until they respawn, from the scoreboard's own respawn times" },
	{ "respawn_timers_teammates", "list your own team's dead too" },
	{ "respawn_timers_pos", "how far down the screen the list starts" },
	{ "pack_timers_active", "count down where a health or ammo pack was taken, it comes back after 10 seconds. only packs you saw get taken are timed" },
	{ "sentry_ranges_active", "draw the range of enemy sentries on the ground, red when the sentry can see you" },
	{ "sentry_ranges_only_near", "only draw the ranges of sentries you're close to" },
	{ "sentry_warning", "a warning on screen while a sentry can see you" },
	{ "sticky_zones_active", "draw the blast radius of enemy stickies, red when one could hit you" },
	{ "sticky_warning", "a warning on screen while you're in range of an armed sticky" },
	{ "sniper_warning", "a warning on screen while a scoped enemy sniper is aiming at you" },
	{ "freecam_active", "fly around while dead. wasd to move, space and ctrl for up and down, shift to go faster" },
	{ "freecam_speed", "units per second" },
	{ "skins_active", "war paints on your own weapons, only you see them. changing them reloads the entities, so there's a short hitch" },
	{ "skins_wear", "how worn the paint looks" },
	{ "skins_primary", "the list shows the paints your equipped primary can take" },
	{ "skins_secondary", "the list shows the paints your equipped secondary can take" },
	{ "skins_melee", "the list shows the paints your equipped melee can take" },

	{ "fakelag_break_lc", "choke long enough to move outside of lag compensation range" },
	{ "seed_pred_resync_interval", "how often to resync the server time used for prediction" },

	{ "esp_healthbar_divisions", "splits the health bar into segments, 1 for a solid bar" },

	{ "spectator_list_override_firstperson", "use the override color for people spectating you in first person" },
	{ "tp_override_force", "always use thirdperson, not just while taunting" },
	{ "skybox_override", "name of a skybox to load instead of the map's, empty to disable" },
	{ "mat_mod_night_scale", "how dark the world gets in night mode" },
	{ "notifs_print_to_con", "also print notifications to the game console" },

	{ "misc_remove_fix_angle", "ignore the server snapping your view angles" },
	{ "misc_remove_teammate_pushaway", "stop teammates pushing you around" },
	{ "misc_taunts_slide", "allow moving while taunting" },
	{ "misc_taunts_control", "steer taunt slides with your movement keys" },

	{ "region_selector_active", "only matchmake into regions that aren't disabled below" },

	{ "ui_color_base", "base color the menu theme is generated from" },
	{ "ui_accent_custom", "use the accent color for checkmarks, sliders, the selected tab and selected items instead of a shade of the base color" },
	{ "ui_rounding", "how round the corners of windows, sections and buttons are" },
	{ "ui_border_size", "thickness of the outlines around windows, sections and buttons, 0 for none" },
	{ "ui_padding", "space between the edge of a window or section and what's inside it" },
	{ "ui_bg_style", "what's behind the menu: a flat color, a gradient into the gradient color, or an image" },
	{ "ui_color_bg_gradient", "color the background fades into when it's set to gradient" },
	{ "ui_bg_image", "image shown behind the menu when the background is set to image" },
	{ "ui_bg_image_opacity", "how strongly the background image shows through" },
	{ "ui_bg_image_fit", "cover crops the image to fill the menu, stretch squashes it to fit" },
	{ "ui_color_accent", "highlight color, used when custom accent is on" },
	{ "ui_tooltip_details", "show every setting's value, default and binds when hovering it" },
	{ "ui_scroll_edit", "scroll over a slider or dropdown to change it, hold shift for bigger steps" },
	{ "ui_slider_buttons", "show -/+ buttons next to sliders for exact steps, hold shift for bigger ones" },
	{ "ui_sounds", "play a quiet click when you click something in the menu" },
	{ "ui_sound_volume", "how loud the click sounds are" },
	{ "ui_animations", "fade the menu in and out and animate switching tabs" },
	{ "ui_context_highlight", "while in game, color the sections, settings and tabs that matter for your class and the weapon you're holding" },
	{ "ui_connection_info", "show ping, tickrate, loss and choke in the menu footer, hover it for details and a graph" },
	{ "ui_bg_opacity", "how see-through the menu and its windows are" },
	{ "ui_fade_unhovered", "fade the whole menu while the mouse isn't over it" },
	{ "ui_fade_opacity", "how faded the menu gets while the mouse is elsewhere" },
	{ "ui_font", "font the menu is drawn with, hover one in the list to preview it" },
	{ "ui_font_size", "font size before the ui scale is applied" },
	{ "ui_font_smoothing", "sharp is pixel perfect, smooth is anti-aliased like the rest of windows" },
	{ "ui_font_letter_spacing", "extra space between letters" },
	{ "ui_font_row_spacing", "extra space between rows of settings" },
	{ "ui_scale", "size of the menu and its text, applied once you let go of the slider" },
};

const char *getTooltip(std::string_view cfg_var_name)
{
	const auto it{ tooltips.find(cfg_var_name) };

	return it != tooltips.end() ? it->second : nullptr;
}
