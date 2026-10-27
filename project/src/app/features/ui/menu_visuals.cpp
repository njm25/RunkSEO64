#include "menu.hpp"
#include "widgets.hpp"
#include "ui.hpp"
#include "ui_fonts.hpp"

#include "../notifs/notifs.hpp"
#include "../entity_visuals/entity_visuals.hpp"
#include "../cfg.hpp"

static ImVec4 toVec4(const Color clr)
{
	return { clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f, clr.a / 255.0f };
}

//hovering a theme previews it on the whole menu, right click deletes saved ones
static void themeCombo()
{
	const std::string current{ ui->themeName(ui->currentTheme()) };

	if (ImGui::BeginCombo("theme", current.c_str(), ImGuiComboFlags_HeightLarge))
	{
		const float swatch{ ImGui::GetTextLineHeight() };

		std::string to_delete{};

		for (const Theme &theme : ui->getThemes())
		{
			ImGui::PushID(theme.name.c_str());

			ImGui::ColorButton("##swatch", toVec4(theme.values.base), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, { swatch, swatch });

			//themes with their own accent show it next to the base color
			if (theme.values.accent_custom) {
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::ColorButton("##accent", toVec4(theme.values.accent), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, { swatch * 0.5f, swatch });
			}

			ImGui::SameLine(swatch * 2.0f + ImGui::GetStyle().ItemSpacing.x);

			if (ImGui::Selectable(theme.name.c_str(), theme.name == current)) {
				ui->applyTheme(theme.values);
			}

			if (ImGui::IsItemHovered()) {
				ui->previewTheme(theme.values);
			}

			if (!theme.preset && ImGui::BeginPopupContextItem("theme_popup_context"))
			{
				if (ImGui::MenuItem("delete")) {
					to_delete = theme.name;
				}

				ImGui::EndPopup();
			}

			ImGui::PopID();
		}

		ImGui::EndCombo();

		if (!to_delete.empty()) {
			confirm(std::format("delete the theme '{}'?", to_delete), [to_delete]() { ui->deleteTheme(to_delete); });
		}
	}
}

//every window back where it starts out, for when one ends up off screen
static void resetLayout()
{
	for (ConfigVar *const var : { &make_cfgvar::ui_pos_menu_x, &make_cfgvar::ui_pos_menu_y, &make_cfgvar::ui_menu_w, &make_cfgvar::ui_menu_h,
		&make_cfgvar::ui_pos_binds_x, &make_cfgvar::ui_pos_binds_y, &make_cfgvar::ui_pos_minimap_x, &make_cfgvar::ui_pos_minimap_y,
		&make_cfgvar::spectator_list_x, &make_cfgvar::spectator_list_y })
	{
		var->reset();
	}

	notifs->message("reset window layout");
}

static const char *const font_sample{ "the quick brown fox jumps over the lazy dog 0123456789" };

static bool containsNoCase(std::string_view haystack, std::string_view needle)
{
	return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(), [](const char a, const char b) {
		return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
	}) != haystack.end();
}

static bool sameFontFile(std::string_view a, std::string_view b)
{
	return a.size() == b.size() && containsNoCase(a, b);
}

//one entry in the font picker, hovering it loads the font so a sample can be shown in it
static void fontEntry(const FontFile &font)
{
	if (ImGui::Selectable(font.name.c_str(), sameFontFile(font.file, cfg::ui_font))) {
		cfg::ui_font = font.file;
	}

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
	{
		ui->previewFont(font.file);

		ImGui::BeginTooltip();

		if (ImFont *const preview{ ui->getPreviewFont(font.file) }) {
			ImGui::PushFont(preview);
			ImGui::TextUnformatted(font_sample);
			ImGui::PopFont();
		}

		else {
			ImGui::TextDisabled("loading preview...");
		}

		ImGui::TextDisabled("%s", font.file.c_str());

		ImGui::EndTooltip();
	}
}

static void fontPicker()
{
	if (ImGui::BeginCombo("font", fontDisplayName(cfg::ui_font).c_str(), ImGuiComboFlags_HeightLarge))
	{
		static std::string filter{};

		if (ImGui::IsWindowAppearing()) {
			filter.clear();
			ImGui::SetKeyboardFocusHere();
		}

		ImGui::SetNextItemWidth(-FLT_MIN);
		inputStringWithHint("##font_filter", &filter, "filter");

		const std::vector<FontFile> &fonts{ installedFonts() };

		//the ones that suit a dense menu, up top
		static const char *const recommended[]{ "tahoma.ttf", "segoeui.ttf", "verdana.ttf", "bahnschrift.ttf", "consola.ttf" };

		ImGui::PushID("recommended");

		bool any_recommended{};

		for (const char *const file : recommended)
		{
			for (const FontFile &font : fonts)
			{
				if (sameFontFile(font.file, file) && containsNoCase(font.name, filter)) {
					fontEntry(font);
					any_recommended = true;
				}
			}
		}

		ImGui::PopID();

		if (any_recommended) {
			ImGui::Separator();
		}

		ImGui::PushID("all");

		for (const FontFile &font : fonts)
		{
			if (containsNoCase(font.name, filter)) {
				fontEntry(font);
			}
		}

		ImGui::PopID();

		if (fonts.empty()) {
			ImGui::TextDisabled("no fonts found");
		}

		if (ImGui::Selectable("rescan fonts folder", false, ImGuiSelectableFlags_NoAutoClosePopups)) {
			installedFonts(true);
		}

		ImGui::EndCombo();
	}

	settingContext("font", &cfg::ui_font);
}

//any font file by path, for fonts that aren't installed
static void customFontInput()
{
	static std::string path{};

	inputStringWithHint("##custom_font", &path, "or a .ttf/.otf path");

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
		ImGui::SetTooltip("full path to a font file, press enter to use it");
	}

	if (!(ImGui::IsItemDeactivated() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))) || path.empty()) {
		return;
	}

	//pasted paths often come quoted
	std::erase(path, '"');

	if (const std::string resolved{ resolveFontPath(path) }; resolved.empty() || readFontName(resolved).empty()) {
		notifs->warning(std::format("'{}' isn't a font file", path));
	}

	else {
		cfg::ui_font = path;
		path.clear();
	}
}

static void fontPresetCombo()
{
	const std::string current{ ui->currentFontPresetName() };

	if (ImGui::BeginCombo("preset", current.c_str()))
	{
		std::string to_delete{};

		for (const FontPreset &preset : ui->getFontPresets())
		{
			ImGui::PushID(preset.name.c_str());

			if (ImGui::Selectable(preset.name.c_str(), preset.name == current)) {
				ui->applyFontPreset(preset);
			}

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
				ImGui::SetTooltip("%s, %dpx%s", fontDisplayName(preset.font).c_str(), preset.size, preset.smoothing == 0 ? ", sharp" : ", smooth");
			}

			if (!preset.preset && ImGui::BeginPopupContextItem("font_preset_popup_context"))
			{
				if (ImGui::MenuItem("delete")) {
					to_delete = preset.name;
				}

				ImGui::EndPopup();
			}

			ImGui::PopID();
		}

		ImGui::EndCombo();

		if (!to_delete.empty()) {
			confirm(std::format("delete the font preset '{}'?", to_delete), [to_delete]() { ui->deleteFontPreset(to_delete); });
		}
	}
}

static void fontPresetSaveInput()
{
	static std::string name{};

	inputStringWithHint("##font_preset_name", &name, "save as preset");

	if (!(ImGui::IsItemDeactivated() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))) || name.empty()) {
		return;
	}

	const FontPreset *const existing{ ui->findFontPreset(name) };

	if (existing && existing->preset) {
		notifs->warning(std::format("'{}' is a built in preset", name));
		return;
	}

	auto save = [preset_name = name]()
	{
		ui->saveFontPreset(preset_name);
		notifs->message(std::format("saved font preset '{}'", preset_name));
	};

	if (existing) {
		confirm(std::format("overwrite the font preset '{}'?", name), save);
	}

	else {
		save();
	}

	name.clear();
}

static void themeSaveInput()
{
	static std::string name{};

	inputStringWithHint("##theme_name", &name, "save as theme");

	if (!(ImGui::IsItemDeactivated() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))) || name.empty()) {
		return;
	}

	const Theme *const existing{ ui->findTheme(name) };

	if (existing && existing->preset) {
		notifs->warning(std::format("'{}' is a built in theme", name));
		return;
	}

	const std::string theme_name{ name };
	const ThemeValues values{ ui->currentTheme() };

	auto save = [theme_name, values]()
	{
		ui->saveTheme(theme_name, values);
		notifs->message(std::format("saved theme '{}'", theme_name));
	};

	if (existing) {
		confirm(std::format("overwrite the theme '{}'?", theme_name), save);
	}

	else {
		save();
	}

	name.clear();
}

void Menu::tabVisuals()
{
	if (ImGui::BeginTabBar("tabbar_visuals"))
	{
		tabItem("entities", m_visuals_tab, VisualsTab::ENTITIES);
		tabItem("map", m_visuals_tab, VisualsTab::MAP);
		tabItem("other", m_visuals_tab, VisualsTab::OTHER);
		tabItem("colors", m_visuals_tab, VisualsTab::COLORS);

		if (m_visuals_tab == VisualsTab::ENTITIES) {
			pageVisualsEntities();
		}

		else if (m_visuals_tab == VisualsTab::MAP) {
			pageVisualsMap();
		}

		else if (m_visuals_tab == VisualsTab::OTHER) {
			pageVisualsOther();
		}

		else if (m_visuals_tab == VisualsTab::COLORS) {
			pageVisualsColors();
		}

		ImGui::EndTabBar();
	}
}

void Menu::sectionEspSettings()
{
	section("esp_settings", "esp settings", 0.0f, []
	{
		selectSingle("name pos", &cfg::esp_name_pos, { { "name on top", 0 }, { "name on right", 1 } });
		selectSingle("text color", &cfg::esp_text_color, { { "default text color", 0 }, { "custom text color", 1 } });
		selectSingle("hp color", &cfg::esp_health_color, { { "default hp color", 0 }, { "custom hp color", 1 } });
		sliderInt("hp bar divisions", &cfg::esp_healthbar_divisions, 1, 10, "%d hp bar divisions");

		sep();
		heading("outline settings");
		sep();

		selectSingle("outlines style", &cfg::outlines_style, { { "blur", 0 }, { "sharp", 1 }, { "thick", 2 } });
		sliderInt("bloom", &cfg::outlines_bloom_amount, 1, 10, "%d bloom");
		toggle("fill occluded", &cfg::outlines_fill_occluded);
	}, -1.0f);
}

void Menu::pageVisualsEntities()
{
	const std::shared_lock<std::shared_mutex> lock(entity_visuals->getMutex());

	ImGui::BeginChild("group_entities", { 0.0f, -m_footer_h }, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
	{
		static EVGroup *temp_group{};

		ImGui::BeginChild("group_entities_first", { scaled(145.0f), 0.0f });
		{
			ImGui::BeginChild("group_entities_groups", { scaled(145.0f), ImGui::GetContentRegionAvail().y * 0.51f }, ImGuiChildFlags_Border);
			{
				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

				ImGui::Text("groups");
				ImGui::Separator();

				static std::string new_group_name{};

				if (inputStringWithHint("##new_group", &new_group_name, "new group", "%s", ImGuiInputTextFlags_EnterReturnsTrue))
				{
					if (new_group_name.size())
					{

						EVGroup group{};
						group.name = new_group_name;

						entity_visuals->pushGroup(group);
						entity_visuals->save(utils::getFilePath() + "entity_visuals.json");

						temp_group = &entity_visuals->getGroups().back();
						new_group_name.clear();
					}
				}

				ImGui::Separator();

				std::vector<EVGroup> &groups{ entity_visuals->getGroups() };

				static size_t dragged_group_index{};

				bool any_group_hovered{};

				for (size_t i{}; i < groups.size(); ++i)
				{
					EVGroup &group{ groups[i] };

					const bool is_active{ temp_group == &group };

					ImGui::PushID(&group);

					if (is_active) {
						ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
					}

					ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.0f, 0.5f });

					if (ImGui::Button(group.name.c_str(), ImVec2{ ImGui::GetContentRegionAvail().x, 0.0f })) {
						temp_group = &group;
					}

					ImGui::PopStyleVar();

					if (is_active) {
						ImGui::PopStyleColor(3);
					}

					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
					{
						dragged_group_index = i;

						ImGui::SetDragDropPayload("GROUP_REORDER", &i, sizeof(size_t));
						ImGui::Text("moving %s", group.name.c_str());
						ImGui::EndDragDropSource();
					}

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload *const payload{ ImGui::AcceptDragDropPayload("GROUP_REORDER") })
						{
							const size_t src_index{ *(size_t *)payload->Data };

							if (src_index != i)
							{
								std::swap(groups[src_index], groups[i]);

								if (temp_group == &groups[src_index]) {
									temp_group = &groups[i];
								}

								else if (temp_group == &groups[i]) {
									temp_group = &groups[src_index];
								}

								entity_visuals->save(utils::getFilePath() + "entity_visuals.json");
							}
						}

						ImGui::EndDragDropTarget();
					}

					if (ImGui::IsItemHovered()) {
						any_group_hovered = true;
					}

					if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
						ImGui::OpenPopup("group_context_menu");
					}

					if (ImGui::BeginPopup("group_context_menu"))
					{
						if (ImGui::MenuItem("delete")) {
							entity_visuals->deleteGroup(group.name);
							temp_group = nullptr;
						}

						ImGui::EndPopup();
					}

					ImGui::PopID();
				}

				if (!any_group_hovered && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					temp_group = nullptr;
				}

				ImGui::PopItemWidth();
			}
			ImGui::EndChild();

			sectionEspSettings();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("group_entities_classes", { scaled(145.0f), 0.0f }, ImGuiChildFlags_Border);
		{
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

			ImGui::Text("classes");
			ImGui::Separator();

			auto getClassName = [](const EVClass c) -> const char *const
			{
				switch (c)
				{
					case EVClass::PLAYER: {
						return "player";
					}

					case EVClass::SENTRYGUN: {
						return "sentrygun";
					}

					case EVClass::DISPENSER: {
						return "dispenser";
					}

					case EVClass::TELEPORTER: {
						return "teleporter";
					}

					case EVClass::PROJECTILE: {
						return "projectile";
					}

					case EVClass::HEALTHPACK: {
						return "healthpack";
					}

					case EVClass::AMMOPACK: {
						return "ammopack";
					}

					case EVClass::HALLOWEEN_GIFTS: {
						return "halloween gifts";
					}

					case EVClass::MVM_MONEY: {
						return "mvm money";
					}

					case EVClass::CTF_FLAGS: {
						return "ctf flags";
					}

					default: {
						return "unknown";
					}
				}
			};

			auto classCheckbox = [&](const EVClass cur_class)
			{
				const auto as_int{ static_cast<int>(cur_class) };

				bool has_class{};

				if (temp_group) {
					has_class = temp_group->classes & as_int;
				}

				if (ImGui::Checkbox(getClassName(cur_class), &has_class))
				{
					if (temp_group)
					{
						if (has_class) {
                                                temp_group->classes |= as_int;
						}

						else {
                                                temp_group->classes &= ~as_int;
						}

						entity_visuals->save(utils::getFilePath() + "entity_visuals.json");
					}
				}
			};

			if (temp_group)
			{
				classCheckbox(EVClass::PLAYER);
				classCheckbox(EVClass::SENTRYGUN);
				classCheckbox(EVClass::DISPENSER);
				classCheckbox(EVClass::TELEPORTER);
				classCheckbox(EVClass::PROJECTILE);
				classCheckbox(EVClass::HEALTHPACK);
				classCheckbox(EVClass::AMMOPACK);
				classCheckbox(EVClass::HALLOWEEN_GIFTS);
				classCheckbox(EVClass::MVM_MONEY);
				classCheckbox(EVClass::CTF_FLAGS);
			}

			ImGui::PopItemWidth();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("group_entities_conditions", { scaled(145.0f), 0.0f }, ImGuiChildFlags_Border);
		{
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

			ImGui::Text("conditions");
			ImGui::Separator();

			auto getCondName = [](const EVCond c) -> const char *const
			{
				switch (c)
				{
					case EVCond::NONE: {
						return "none";
					}

					case EVCond::LOCAL: {
						return "local";
					}

					case EVCond::TEAMMATE: {
						return "teammate";
					}

					case EVCond::ENEMY: {
						return "enemy";
					}
					
					case EVCond::FRIEND: {
						return "friend";
					}

					case EVCond::INVISIBLE: {
						return "invisible";
					}

					case EVCond::INVULNERABLE: {
						return "invulnerable";
					}

					case EVCond::AIMBOT_TARGET: {
						return "aimbot target";
					}

					case EVCond::CLASS_SCOUT: {
						return "class scout";
					}

					case EVCond::CLASS_SOLDIER: {
						return "class soldier";
					}

					case EVCond::CLASS_PYRO: {
						return "class pyro";
					}

					case EVCond::CLASS_DEMOMAN: {
						return "class demoman";
					}

					case EVCond::CLASS_HEAVY: {
						return "class heavy";
					}

					case EVCond::CLASS_ENGINEER: {
						return "class engineer";
					}

					case EVCond::CLASS_MEDIC: {
						return "class medic";
					}

					case EVCond::CLASS_SNIPER: {
						return "class sniper";
					}

					case EVCond::CLASS_SPY: {
						return "class spy";
					}

					default: {
						return "unknown";
					}
				}
			};

			auto condCheckbox = [&](const EVCond c)
			{
				const auto as_int{ static_cast<int>(c) };

				bool has_cond{};

				if (temp_group) {
					has_cond = temp_group->conds & as_int;
				}

				if (ImGui::Checkbox(getCondName(c), &has_cond))
				{
					if (temp_group)
					{
						if (has_cond) {
                                                temp_group->conds |= as_int;
						}

						else {
                                                temp_group->conds &= ~as_int;
						}

						entity_visuals->save(utils::getFilePath() + "entity_visuals.json");
					}
				}
			};

			if (temp_group)
			{
                                    ImGui::Separator();

				condCheckbox(EVCond::LOCAL);
				condCheckbox(EVCond::TEAMMATE);
				condCheckbox(EVCond::ENEMY);
				condCheckbox(EVCond::FRIEND);

				ImGui::Separator();

				condCheckbox(EVCond::INVISIBLE);
				condCheckbox(EVCond::INVULNERABLE);

				ImGui::Separator();

				condCheckbox(EVCond::AIMBOT_TARGET);

				ImGui::Separator();

				condCheckbox(EVCond::CLASS_SCOUT);
				condCheckbox(EVCond::CLASS_SOLDIER);
				condCheckbox(EVCond::CLASS_PYRO);
				condCheckbox(EVCond::CLASS_DEMOMAN);
				condCheckbox(EVCond::CLASS_HEAVY);
				condCheckbox(EVCond::CLASS_ENGINEER);
				condCheckbox(EVCond::CLASS_MEDIC);
				condCheckbox(EVCond::CLASS_SNIPER);
				condCheckbox(EVCond::CLASS_SPY);
			}

			ImGui::PopItemWidth();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("group_entities_effects", { scaled(145.0f), 0.0f }, ImGuiChildFlags_Border);
		{
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

			ImGui::Text("effects");
			ImGui::Separator();

			auto getEffectName = [](const EVEffect e) -> const char *const
			{
				switch (e)
				{
					case EVEffect::NAME: {
						return "name";
					}

					case EVEffect::BOX: {
						return "box";
					}

					case EVEffect::HP: {
						return "health";
					}

					case EVEffect::HPBAR: {
						return "health bar";
					}

					case EVEffect::MATERIAL: {
						return "material";
					}

					case EVEffect::OUTLINE: {
						return "outline";
					}

					case EVEffect::CLASS: {
						return "class";
					}

					case EVEffect::CLASSICON: {
						return "class icon";
					}

					case EVEffect::TAGS: {
						return "tags";
					}

					case EVEffect::UBER: {
						return "uber";
					}

					case EVEffect::UBERBAR: {
						return "uber bar";
					}

					case EVEffect::STEAMID: {
						return "steam id";
					}

					case EVEffect::CONDITIONS: {
						return "conditions";
					}
					
					case EVEffect::WEAPONS: {
						return "weapons";
					}

					default: {
						return "unknown";
					}
				}
			};

			auto effectCheckbox = [&](const EVEffect e)
			{
				const auto as_int{ static_cast<int>(e) };

				bool has_effect{};

				if (temp_group) {
					has_effect = temp_group->effects & as_int;
				}

				if (ImGui::Checkbox(getEffectName(e), &has_effect))
				{
					if (temp_group)
					{
						if (has_effect) {
                                                temp_group->effects |= as_int;
                                            }

                                            else {
                                                temp_group->effects &= ~as_int;
						}

						entity_visuals->save(utils::getFilePath() + "entity_visuals.json");
					}
				}
			};

			if (temp_group)
			{
				ImGui::BeginChild("group_visuals_entities_effects", { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.55f }, ImGuiChildFlags_Border);
				{
					effectCheckbox(EVEffect::NAME);
					effectCheckbox(EVEffect::BOX);
					effectCheckbox(EVEffect::HP);
					effectCheckbox(EVEffect::HPBAR);
					effectCheckbox(EVEffect::CLASS);
					effectCheckbox(EVEffect::CLASSICON);
					effectCheckbox(EVEffect::TAGS);
					effectCheckbox(EVEffect::UBER);
					effectCheckbox(EVEffect::UBERBAR);
					effectCheckbox(EVEffect::STEAMID);
					effectCheckbox(EVEffect::WEAPONS);

					if (temp_group->effects & static_cast<int>(EVEffect::WEAPONS)) {
						auto getWeaponType = [](const EVWeaponType t) -> const char *const
						{
							switch (t)
							{
								case EVWeaponType::ACTIVE_NAME: {
									return "active name";
								}

								case EVWeaponType::ACTIVE_ICON: {
									return "active icon";
								}

								case EVWeaponType::ALL_NAMES: {
									return "all names";
								}

								default: {
									return "unknown";
								}
							}
						};

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

						ImGui::Separator();

						if (ImGui::BeginCombo("##wep_type", temp_group ? getWeaponType(temp_group->weapon_type) : "select weapon type"))
						{
							for (int i{}; i < static_cast<int>(EVWeaponType::COUNT); i++)
							{
								const EVWeaponType t{ static_cast<EVWeaponType>(i) };

								if (ImGui::Selectable(getWeaponType(t), temp_group && temp_group->weapon_type == t))
								{
									if (temp_group) {
										temp_group->weapon_type = t;

										entity_visuals->save(utils::getFilePath() + "entity_visuals.json");
									}
								}
							}

							ImGui::EndCombo();
						}

						ImGui::PopItemWidth();
					}

					ImGui::Separator();

					ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

					if (sliderFloat("##esp_alpha", &temp_group->esp_alpha, 0.01f, 1.0f, "%.2f")) {
						entity_visuals->save(utils::getFilePath() + "entity_visuals.json");
					}

					ImGui::PopItemWidth();
				}
				ImGui::EndChild();

				ImGui::Separator();

				ImGui::BeginChild("group_visuals_entities_materials", { ImGui::GetContentRegionAvail().x, 0.0f }, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
				{
					ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

					auto getMatType = [](const EVMaterialType t) -> const char *const
					{
						switch (t)
						{
							case EVMaterialType::ORIGINAL: {
								return "original";
							}

							case EVMaterialType::FLAT: {
								return "flat";
							}

							case EVMaterialType::SHADED: {
								return "shaded";
							}

							case EVMaterialType::GLOSSY: {
								return "glossy";
							}

							case EVMaterialType::GLOW: {
								return "glow";
							}

							case EVMaterialType::PLASTIC: {
								return "plastic";
							}

							default: {
								return "unknown";
							}
						}
					};

					effectCheckbox(EVEffect::MATERIAL);

					ImGui::Separator();

					if (toggle("depth", &temp_group->mat_depth)) {
						entity_visuals->save(utils::getFilePath() + "entity_visuals.json");
					}

					ImGui::Separator();

					if (ImGui::BeginCombo("##mat_type", temp_group ? getMatType(temp_group->mat_type) : "select material type"))
					{
						for (int i{}; i < static_cast<int>(EVMaterialType::COUNT); i++)
						{
							const EVMaterialType t{ static_cast<EVMaterialType>(i) };

							if (ImGui::Selectable(getMatType(t), temp_group && temp_group->mat_type == t))
							{
								if (temp_group) {
									temp_group->mat_type = t;

									entity_visuals->save(utils::getFilePath() + "entity_visuals.json");
								}
							}
						}

						ImGui::EndCombo();
					}

					if (sliderFloat("##mat_alpha", &temp_group->mat_alpha, 0.01f, 1.0f, "%.2f")) {
						entity_visuals->save(utils::getFilePath() + "entity_visuals.json");
					}

					ImGui::PopItemWidth();
				}
				ImGui::EndChild();

				ImGui::Separator();

				ImGui::BeginChild("group_visuals_entities_effects_outline", { ImGui::GetContentRegionAvail().x, 0.0f }, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
				{
					ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

					effectCheckbox(EVEffect::OUTLINE);

					ImGui::Separator();

					if (sliderFloat("##outline_alpha", &temp_group->outline_alpha, 0.01f, 1.0f, "%.2f")) {
						entity_visuals->save(utils::getFilePath() + "entity_visuals.json");
					}

					ImGui::PopItemWidth();
				}
				ImGui::EndChild();
			}

			ImGui::PopItemWidth();
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
}

void Menu::pageVisualsMap()
{
	beginPage("group_map", m_footer_h);
	{
		beginColumn("group_map0");
		{
			section("map_global", "global", 180.0f, []
			{
				toggle("active", &cfg::map_active);

				sep();

				toggle("draw nav", &cfg::map_draw_nav);
				toggle("highlight height", &cfg::map_nav_highlight_height);
				colorPicker("nav color", &cfg::map_nav_color);

				sep();

				sliderFloat("width", &cfg::map_width, 100.0f, 1000.0f, "%.0f");
				sliderFloat("height", &cfg::map_height, 100.0f, 1000.0f, "%.0f");

				sep();

				sliderFloat("zoom", &cfg::map_zoom, 0.1f, 1.0f, "%.2f");
				sliderFloat("origin", &cfg::map_origin, 0.5f, 0.95f, "%.2f");
				sliderFloat("perspective", &cfg::map_perspective, 0.0f, 2.0f, "%.2f");
			});

			nextColumn();

			section("map_players", "players", 180.0f, []
			{
				toggle("active", &cfg::map_players_active);

				selectSingle("style", &cfg::map_players_style,
				{
					{ "circle", 0 },
					{ "arrow", 1 }
				});

				sep();

				selectMulti("ignore",
				{
					{ "friend", &cfg::map_players_ignore_friends },
					{ "local", &cfg::map_players_ignore_local },
					{ "enemies", &cfg::map_players_ignore_enemies },
					{ "teammates", &cfg::map_players_ignore_teammates }
				});
			});
		}
		endColumn();
	}
	endPage();
}

void Menu::pageVisualsOther()
{
	beginPage("group_visuals_other", m_footer_h);
	{
		beginColumn("group_visuals_other0");
		{
			section("specator_list", "specator list", 180.0f, []
			{
				toggle("active", &cfg::spectator_list_active);
				toggle("show respawn time", &cfg::spectator_list_show_respawn_time);

				sep();

				toggle("override firstperson color", &cfg::spectator_list_override_firstperson);
				colorPicker("override color", &cfg::spectator_list_firstperson_clr);
			});

			section("fov_override", "fov override", 180.0f, []
			{
				toggle("active", &cfg::fov_override_active);

				sep();

				sliderInt("value", &cfg::fov_override_value, 1, 180);
				sliderInt("value zoomed", &cfg::fov_override_value_zoomed, 1, 180);
			});

			section("viewmodel_override", "viewmodel override", 180.0f, []
			{
				toggle("active", &cfg::vm_override_active);

				sep();

				sliderFloat("offset x", &cfg::vm_override_offset_x, -50.0f, 50.0f, "%.1f");
				sliderFloat("offset y", &cfg::vm_override_offset_y, -50.0f, 50.0f, "%.1f");
				sliderFloat("offset z", &cfg::vm_override_offset_z, -50.0f, 50.0f, "%.1f");

				sep();

				sliderFloat("sway", &cfg::vm_override_sway, 0.0f, 100.0f, "%.0f%%");
			});

			section("removals", "removals", 180.0f, []
			{
				toggle("remove scope texture", &cfg::remove_scope);
				toggle("remove view punch", &cfg::remove_punch);
				toggle("remove view shake", &cfg::remove_shake);
				toggle("remove view fade", &cfg::remove_fade);
				toggle("remove post processing", &cfg::remove_post_processing);
				toggle("remove fog", &cfg::remove_fog);
				toggle("remove sky fog", &cfg::remove_sky_fog);
				toggle("remove hands", &cfg::remove_hands);
				toggle("remove visual interp", &cfg::remove_interp);
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_visuals_other1");
		{
			section("aspect_ratio", "aspect ratio", 180.0f, []
			{
				toggle("active", &cfg::aspect_ratio_active);

				sep();

				sliderFloat("value", &cfg::aspect_ratio_value, 1.0f, 2.0f, "%.2f");
			});

			section("thirdperson_override", "thirdperson", 180.0f, []
			{
				toggle("active", &cfg::tp_override_active);
				toggle("force", &cfg::tp_override_force);
				toggle("collision", &cfg::tp_override_collision);

				sep();

				sliderFloat("dist forward", &cfg::tp_override_dist_forward, 50.0f, 300.0f, "%.0f");
				sliderFloat("dist right", &cfg::tp_override_dist_right, -50.0f, 50.0f, "%.0f");
				sliderFloat("dist up", &cfg::tp_override_dist_up, -50.0f, 50.0f, "%.0f");

				sep();

				sliderFloat("lerp time", &cfg::tp_override_lerp_time, 0.0f, 0.5f, "%.2fs");
			});

			section("particle_modulation", "particle modulation", 180.0f, []
			{
				toggle("active", &cfg::particle_mod_active);

				sep();

				selectSingle("mode", &cfg::particle_mod_mode, { { "color", 0 }, { "rainbow", 1 } });
				sliderFloat("rainbow speed", &cfg::particle_mod_rainbow_speed, 0.1f, 5.0f, "%.2f");

				sep();

				colorPicker("color", &cfg::particle_mod_color);
			});

			section("proj_sim_visuals", "draw projectile arc", 180.0f, []
			{
				toggle("active", &cfg::proj_sim_visual_active);

				sep();

				selectSingle("mode", &cfg::proj_sim_visual_mode, { { "color", 0 }, { "rainbow", 1 } });
				sliderFloat("rainbow speed", &cfg::proj_sim_visual_rainbow_speed, 0.1f, 5.0f, "%.2f");

				sep();

				colorPicker("color", &cfg::proj_sim_visual_color);
			});

			section("notifications", "notifications", 180.0f, []
			{
				toggle("active", &cfg::notifs_active);
				toggle("print to console", &cfg::notifs_print_to_con);

				sep();

				sliderFloat("duration", &cfg::notifs_duration, 1.0f, 10.0f, "%.1fs");
				sliderFloat("fade in time", &cfg::notifs_fade_in_time, 0.0f, 0.1f, "%.2fs");
				sliderFloat("fade out time", &cfg::notifs_fade_out_time, 0.0f, 0.1f, "%.2fs");
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_visuals_other2");
		{
			section("material_modulation", "material modulation", 180.0f, []
			{
				toggle("active", &cfg::mat_mod_active);

				sep();

				selectSingle("mode", &cfg::mat_mod_mode, { { "night", 0 }, { "color", 1 } });
				sliderFloat("night scale", &cfg::mat_mod_night_scale, 0.0f, 1.0f, "%.2f");

				sep();

				colorPicker("world", &cfg::mat_mod_color_world);
				colorPicker("props", &cfg::mat_mod_color_props);
				colorPicker("sky", &cfg::mat_mod_color_sky);
			});

			section("tracers", "tracers", 180.0f, []
			{
				toggle("active", &cfg::tracers_active);
				toggle("beam fade", &cfg::tracers_fade);

				sep();

				selectSingle("type", &cfg::tracers_type, { { "beam", 0 }, { "white machina", 1 }, { "team machina", 2}, {"team capper", 3} });

				sep();

				selectSingle("color mode", &cfg::tracers_mode, { { "color", 0 }, { "rainbow", 1 } });
				sliderFloat("rainbow speed", &cfg::tracers_rainbow_speed, 0.1f, 5.0f, "%.2f");
				colorPicker("color", &cfg::tracers_color);
			});

			section("other", "other", 180.0f, []
			{
				toggle("reveal scoreboard", &cfg::reveal_scoreboard);
				toggle("color scoreboard", &cfg::color_scoreboard);
				toggle("static player model lighting", &cfg::static_player_model_lighting);
				toggle("spritecard texture hack", &cfg::spritecard_texture_hack);
				toggle("allow mvm inspect", &cfg::allow_mvm_inspect);
				toggle("flat textures", &cfg::flat_textures);
				toggle("force low lod models", &cfg::force_low_lod_models);
				toggle("crosshair follows aimbot", &cfg::crosshair_follows_aimbot);

				sep();

				selectSingle("sniper lines", &cfg::sniper_sight_lines, { { "off", 0 }, { "laser", 1 }, { "line", 2 } });

				sep();

				inputString("skybox override", &cfg::skybox_override);
			});
		}
		endColumn();
	}
	endPage();
}

void Menu::pageVisualsColors()
{
	beginPage("group_visuals_colors", m_footer_h);
	{
		beginColumn("group_visuals_colors0");
		{
			section("colors_global", "global", 120.0f, []
			{
				colorPicker("local", &cfg::color_local);
				colorPicker("friend", &cfg::color_friend);
				colorPicker("enemy", &cfg::color_enemy);
				colorPicker("teammate", &cfg::color_teammate);
				colorPicker("target", &cfg::color_target);
				colorPicker("invuln", &cfg::color_invuln);
				colorPicker("invis", &cfg::color_invis);
				colorPicker("overheal", &cfg::color_overheal);
				colorPicker("uber", &cfg::color_uber);
				colorPicker("conds", &cfg::color_conds);
				colorPicker("health pack", &cfg::color_health_pack);
				colorPicker("ammo pack", &cfg::color_ammo_pack);
				colorPicker("halloween gift", &cfg::color_halloween_gift);
				colorPicker("mvm money", &cfg::color_mvm_money);
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_visuals_colors1");
		{
			section("colors_esp", "esp", 120.0f, []
			{
				colorPicker("text color", &cfg::color_esp_text);
				colorPicker("low health color", &cfg::color_esp_low_health);
				colorPicker("high health color", &cfg::color_esp_high_health);
			});

			section("colors_notifications", "notifications", 120.0f, []
			{
				colorPicker("message", &cfg::color_notifs_style_message);
				colorPicker("warning", &cfg::color_notifs_style_warning);
			});

			section("colors_indicators", "indicators", 120.0f, []
			{
				colorPicker("extra good", &cfg::color_indicators_extra_good);
				colorPicker("good", &cfg::color_indicators_good);
				colorPicker("mid", &cfg::color_indicators_mid);
				colorPicker("bad", &cfg::color_indicators_bad);
			});

			section("colors_ui", "theme", 120.0f, []
			{
				colorPicker("base", &cfg::ui_color_base);
				toggle("custom accent", &cfg::ui_accent_custom);
				colorPicker("accent", &cfg::ui_color_accent);

				sep();

				if (customWidget("theme")) {
					themeCombo();
				}

				if (customWidget("save as theme")) {
					themeSaveInput();
				}
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_visuals_colors2");
		{
			section("menu", "menu", 180.0f, []
			{
				sliderFloat("scale", &cfg::ui_scale, 0.75f, 2.0f, "%.2fx");
				toggle("scroll to edit", &cfg::ui_scroll_edit);
				toggle("slider buttons", &cfg::ui_slider_buttons);
				toggle("animations", &cfg::ui_animations);

				sep();

				sliderFloat("background opacity", &cfg::ui_bg_opacity, 0.2f, 1.0f, "%.2f");
				toggle("fade when not hovered", &cfg::ui_fade_unhovered);
				sliderFloat("faded opacity", &cfg::ui_fade_opacity, 0.1f, 1.0f, "%.2f");

				sep();

				if (customWidget("reset layout"))
				{
					if (ImGui::Button("reset layout", { ImGui::CalcItemWidth(), 0.0f })) {
						resetLayout();
					}

					if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
						ImGui::SetTooltip("move the menu, binds list and minimap back to their default spots and sizes");
					}
				}
			});

			section("fonts", "fonts", 180.0f, []
			{
				if (customWidget("preset")) {
					fontPresetCombo();
				}

				if (customWidget("save as preset")) {
					fontPresetSaveInput();
				}

				sep();

				if (customWidget("font", &cfg::ui_font)) {
					fontPicker();
				}

				if (customWidget("custom font file", &cfg::ui_font)) {
					customFontInput();
				}

				sliderInt("size", &cfg::ui_font_size, 10, 20, "%dpx");
				selectSingle("smoothing", &cfg::ui_font_smoothing, { { "sharp", 0 }, { "smooth", 1 }, { "smooth (light hinting)", 2 } });
				toggle("bold", &cfg::ui_font_bold);
				toggle("italic", &cfg::ui_font_italic);

				sep();

				sliderFloat("letter spacing", &cfg::ui_font_letter_spacing, -1.0f, 2.0f, "%.0fpx");
				sliderFloat("row spacing", &cfg::ui_font_row_spacing, -2.0f, 2.0f, "%.0fpx");

				if (customWidget("sample")) {
					sep();
					ImGui::TextWrapped("%s", font_sample);
				}
			});
		}
		endColumn();
	}
	endPage();
}
