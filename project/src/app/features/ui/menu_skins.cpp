#include "menu.hpp"
#include "widgets.hpp"
#include "ui.hpp"

#include "../skin_changer/skin_changer.hpp"
#include "../notifs/notifs.hpp"
#include "../cfg.hpp"

#include <fstream>

static bool containsNoCase(std::string_view haystack, std::string_view needle)
{
	return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(), [](const char a, const char b) {
		return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
	}) != haystack.end();
}

static const char *const class_labels[]{ "all classes", "scout", "sniper", "soldier", "demoman", "medic", "heavy", "pyro", "spy", "engineer" };

static const char *const sheen_names[]{ "none", "team shine", "deadly daffodil", "manndarin", "mean green", "agonizing emerald", "villainous violet", "hot rod" };
static const char *const killstreaker_names[]{ "none", "fire horns", "cerebral discharge", "tornado", "flames", "singularity", "incinerator", "hypno-beam" };
static const char *const unusual_names[]{ "none", "hot", "isotope", "cool", "energy orb" };
static const char *const tier_names[]{ "none", "basic", "specialized", "professional" };
static const char *const wear_names[]{ "factory new", "minimal wear", "field tested", "well worn", "battle scarred" };

//what the weapons tab is looking at, kept between frames
static int selected_base{ -1 };
static std::string list_filter{};
static int class_filter{};
static bool only_overridden{};
static std::optional<WeaponOverride> copied{};

static std::string kitName(const int kit)
{
	const std::string name{ tf_utils::getPaintKitName(kit) };

	return name.empty() ? std::format("paint kit {}", kit) : name;
}

static int slotOrder(const std::string &slot)
{
	if (slot == "primary") {
		return 0;
	}

	if (slot == "secondary") {
		return 1;
	}

	if (slot == "melee") {
		return 2;
	}

	return 3;
}

//combo over a fixed list of names, the index is the value
static bool namedCombo(const char *const label, int *const var, const char *const *const names, const int count)
{
	bool changed{};

	*var = std::clamp(*var, 0, count - 1);

	if (ImGui::BeginCombo(label, names[*var]))
	{
		for (int n{}; n < count; n++)
		{
			if (ImGui::Selectable(names[n], n == *var)) {
				*var = n;
				changed = true;
			}
		}

		ImGui::EndCombo();
	}

	return changed;
}

//greys out the next controls with the reason on hover when the shown variant can't do it
static void disabledReason(const bool disabled, const char *const reason)
{
	if (disabled && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		ImGui::SetTooltip("%s", reason);
	}
}

static void weaponList(const std::vector<int> &equipped)
{
	ImGui::SetNextItemWidth(-FLT_MIN);
	inputStringWithHint("##skins_filter", &list_filter, "filter weapons");

	ImGui::SetNextItemWidth(scaled(110.0f));
	namedCombo("##skins_class", &class_filter, class_labels, static_cast<int>(std::size(class_labels)));

	ImGui::SameLine();
	ImGui::Checkbox("overridden", &only_overridden);

	const auto &overrides{ skin_changer->overrides() };

	std::vector<const SkinFamily *> shown{};

	for (const SkinFamily &family : skin_catalog->families())
	{
		if (class_filter && !(family.classes & (1u << class_filter))) {
			continue;
		}

		if (only_overridden && !overrides.contains(family.base)) {
			continue;
		}

		if (!list_filter.empty() && !containsNoCase(family.name, list_filter)) {
			continue;
		}

		shown.push_back(&family);
	}

	//what you're holding first, then by slot, then by name
	std::stable_sort(shown.begin(), shown.end(), [&](const SkinFamily *a, const SkinFamily *b)
	{
		const bool ea{ std::find(equipped.begin(), equipped.end(), a->base) != equipped.end() };
		const bool eb{ std::find(equipped.begin(), equipped.end(), b->base) != equipped.end() };

		if (ea != eb) {
			return ea;
		}

		return slotOrder(a->slot) < slotOrder(b->slot);
	});

	ImGui::BeginChild("##skins_list", { 0.0f, scaled(360.0f) }, ImGuiChildFlags_Border);
	{
		int last_group{ -2 };

		for (const SkinFamily *const family : shown)
		{
			const bool is_equipped{ std::find(equipped.begin(), equipped.end(), family->base) != equipped.end() };
			const int group{ is_equipped ? -1 : slotOrder(family->slot) };

			if (group != last_group)
			{
				static const char *const group_names[]{ "primary", "secondary", "melee", "other" };

				ImGui::TextDisabled("%s", group < 0 ? "equipped" : group_names[group]);
				last_group = group;
			}

			const auto it{ overrides.find(family->base) };
			const bool overridden{ it != overrides.end() && it->second.enabled };

			if (is_equipped) {
				ImGui::PushStyleColor(ImGuiCol_Text, highlightColor());
			}

			ImGui::PushID(family->base);

			const std::string label{ std::format("{}{}", overridden ? "* " : "  ", family->name) };

			if (ImGui::Selectable(label.c_str(), selected_base == family->base)) {
				selected_base = family->base;
			}

			ImGui::PopID();

			if (is_equipped) {
				ImGui::PopStyleColor();
			}
		}

		if (shown.empty()) {
			ImGui::TextDisabled("nothing matches");
		}
	}
	ImGui::EndChild();
}

static void warPaintPicker(WeaponOverride &o, const SkinItem &item, bool &changed)
{
	const bool disabled{ item.paintkits.empty() };

	ImGui::BeginDisabled(disabled);

	if (ImGui::BeginCombo("war paint", o.paintkit ? kitName(o.paintkit).c_str() : "none", ImGuiComboFlags_HeightLarge))
	{
		static std::string filter{};

		if (ImGui::IsWindowAppearing()) {
			filter.clear();
			ImGui::SetKeyboardFocusHere();
		}

		ImGui::SetNextItemWidth(-FLT_MIN);
		inputStringWithHint("##kit_filter", &filter, "filter");

		if (ImGui::Selectable("none", o.paintkit == 0)) {
			o.paintkit = 0;
			changed = true;
		}

		//sorted by name, the schema keeps them by id
		std::vector<std::pair<std::string, int>> kits{};

		for (const int kit : item.paintkits) {
			kits.emplace_back(kitName(kit), kit);
		}

		std::sort(kits.begin(), kits.end());

		for (const auto &[name, kit] : kits)
		{
			if (!containsNoCase(name, filter)) {
				continue;
			}

			ImGui::PushID(kit);

			if (ImGui::Selectable(name.c_str(), o.paintkit == kit)) {
				o.paintkit = kit;
				changed = true;
			}

			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	ImGui::EndDisabled();
	disabledReason(disabled, "this variant can't take war paints");

	if (o.paintkit && !disabled)
	{
		changed |= namedCombo("wear", &o.wear, wear_names, static_cast<int>(std::size(wear_names)));
		changed |= ImGui::InputInt("seed", &o.seed);

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("pattern placement, 0 keeps the paint's own");
		}
	}
}

static void variantPicker(WeaponOverride &o, const SkinFamily &family, bool &changed)
{
	const SkinItem *const picked{ o.variant ? skin_catalog->item(o.variant) : nullptr };

	if (ImGui::BeginCombo("variant", picked ? picked->name.c_str() : "automatic", ImGuiComboFlags_HeightLarge))
	{
		if (ImGui::Selectable("automatic", o.variant == 0)) {
			o.variant = 0;
			changed = true;
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("keeps what you have equipped, unless it can't show what's picked below");
		}

		for (const int def : family.members)
		{
			const SkinItem *const item{ skin_catalog->item(def) };

			//decorated ones are just a war paint, the paint picker covers those
			if (!item || item->decorated) {
				continue;
			}

			ImGui::PushID(def);

			if (ImGui::Selectable(item->name.c_str(), o.variant == def)) {
				o.variant = def;
				changed = true;
			}

			ImGui::PopID();
		}

		ImGui::EndCombo();
	}
}

static void stylePicker(WeaponOverride &o, const SkinItem &item, bool &changed)
{
	if (item.styles.size() < 2) {
		return;
	}

	std::string preview{ "item's own" };

	for (const SkinStyle &style : item.styles)
	{
		if (style.index == o.style) {
			preview = style.name;
		}
	}

	if (ImGui::BeginCombo("style", preview.c_str()))
	{
		if (ImGui::Selectable("item's own", o.style < 0)) {
			o.style = -1;
			changed = true;
		}

		for (const SkinStyle &style : item.styles)
		{
			ImGui::PushID(style.index);

			if (ImGui::Selectable(style.name.c_str(), o.style == style.index)) {
				o.style = style.index;
				changed = true;
			}

			ImGui::PopID();
		}

		ImGui::EndCombo();
	}
}

static void botkillerPicker(WeaponOverride &o, const SkinFamily &family, bool &changed)
{
	if (!family.any_botkiller) {
		return;
	}

	const SkinItem *const picked{ o.botkiller ? skin_catalog->item(o.botkiller) : nullptr };

	if (ImGui::BeginCombo("botkiller head", picked ? picked->name.c_str() : "none"))
	{
		if (ImGui::Selectable("none", o.botkiller == 0)) {
			o.botkiller = 0;
			changed = true;
		}

		for (const int def : family.members)
		{
			const SkinItem *const item{ skin_catalog->item(def) };

			if (!item || !item->botkiller) {
				continue;
			}

			ImGui::PushID(def);

			if (ImGui::Selectable(item->name.c_str(), o.botkiller == def)) {
				o.botkiller = def;
				changed = true;
			}

			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("the robot head off a botkiller, on top of whatever else is picked");
	}
}

static void overrideEditor(const std::vector<int> &equipped)
{
	const SkinFamily *const family{ skin_catalog->familyByBase(selected_base) };

	if (!family)
	{
		ImGui::TextDisabled("pick a weapon on the left");
		return;
	}

	const WeaponOverride *const existing{ skin_changer->find(family->base) };

	WeaponOverride o{ existing ? *existing : WeaponOverride{} };

	bool changed{};

	ImGui::TextUnformatted(family->name.c_str());

	if (std::find(equipped.begin(), equipped.end(), family->base) != equipped.end())
	{
		ImGui::SameLine();
		ImGui::TextColored(highlightColor(), "(equipped)");
	}

	if (!existing)
	{
		if (ImGui::Button("add override")) {
			skin_changer->set(family->base, o);
		}

		ImGui::TextDisabled("covers all %zu variants of this weapon", family->members.size());
		return;
	}

	changed |= ImGui::Checkbox("enabled", &o.enabled);

	sep();

	variantPicker(o, *family, changed);

	//the options follow what the weapon would actually be shown as
	const int shown_def{ skin_changer->resolveVariant(*family, o, o.variant ? o.variant : family->base) };
	const SkinItem *const shown{ skin_catalog->item(shown_def) };

	if (!shown) {
		return;
	}

	if (!o.variant) {
		ImGui::TextDisabled("shows as %s", shown->name.c_str());
	}

	//automatic finds a variant that has each option, a hand picked one greys out what it can't do
	const SkinItem &caps{ *shown };
	const bool pick_any{ o.variant == 0 };

	auto can = [&](auto has_family, auto has_item) { return pick_any ? has_family : has_item; };

	sep();

	{
		//automatic can move to any member with paints, so offer the whole family's list through the best one
		const SkinItem *paint_source{ &caps };

		if (pick_any && caps.paintkits.empty())
		{
			for (const int def : family->members)
			{
				const SkinItem *const item{ skin_catalog->item(def) };

				if (item && !item->paintkits.empty() && !item->decorated) {
					paint_source = item;
					break;
				}
			}
		}

		warPaintPicker(o, *paint_source, changed);
	}

	{
		const bool ok{ can(family->any_australium, caps.australium) };

		ImGui::BeginDisabled(!ok);
		changed |= ImGui::Checkbox("australium", &o.australium);
		ImGui::EndDisabled();
		disabledReason(!ok, "no australium version of this weapon");
	}

	{
		const bool ok{ can(family->any_festivizable, caps.festivizable) };

		ImGui::BeginDisabled(!ok);
		changed |= ImGui::Checkbox("festivized", &o.festivized);
		ImGui::EndDisabled();
		disabledReason(!ok, "this weapon can't be festivized");
	}

	{
		const bool family_stattrak{ std::any_of(family->members.begin(), family->members.end(), [](const int def) {
			const SkinItem *const item{ skin_catalog->item(def) };
			return item && item->stattrak;
		}) };

		const bool ok{ can(family_stattrak, caps.stattrak) };

		ImGui::BeginDisabled(!ok);
		changed |= ImGui::Checkbox("strange counter", &o.stattrak);
		ImGui::EndDisabled();
		disabledReason(!ok, "only war paintable weapons carry the counter");
	}

	sep();

	changed |= namedCombo("killstreak", &o.ks_tier, tier_names, static_cast<int>(std::size(tier_names)));

	if (o.ks_tier >= 2) {
		changed |= namedCombo("sheen", &o.sheen, sheen_names, static_cast<int>(std::size(sheen_names)));
	}

	if (o.ks_tier >= 3) {
		changed |= namedCombo("killstreaker", &o.killstreaker, killstreaker_names, static_cast<int>(std::size(killstreaker_names)));
	}

	changed |= namedCombo("unusual", &o.unusual, unusual_names, static_cast<int>(std::size(unusual_names)));

	stylePicker(o, *shown, changed);
	//botkiller heads don't render right yet, picker hidden until they do (see BOTKILLER_HEADS in skin_changer.cpp)
	//botkillerPicker(o, *family, changed);

	//halloween spells, the game only draws them during the halloween/full moon holiday
	if (canPumpkin(*shown) || canGreenFlames(*shown))
	{
		sep();

		if (canPumpkin(*shown)) {
			changed |= ImGui::Checkbox("pumpkin spell", &o.spell_pumpkin);
		}

		if (canGreenFlames(*shown)) {
			changed |= ImGui::Checkbox("halloween fire", &o.spell_green_flames);
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("only shows while the halloween / full moon holiday is on");
		}
	}

	sep();

	if (ImGui::Button("copy")) {
		copied = o;
	}

	ImGui::SameLine();

	ImGui::BeginDisabled(!copied);

	if (ImGui::Button("paste"))
	{
		//the variant and head belong to the weapon it was copied from
		WeaponOverride pasted{ *copied };

		pasted.variant = 0;
		pasted.botkiller = 0;
		o = pasted;
		changed = true;
	}

	ImGui::EndDisabled();

	ImGui::SameLine();

	if (ImGui::Button("remove")) {
		skin_changer->erase(family->base);
		return;
	}

	if (changed && !(o == *existing)) {
		skin_changer->set(family->base, o);
	}
}

void Menu::tabSkins()
{
	beginPage("group_skins", m_footer_h);
	{
		beginColumn("group_skins0");
		{
			section("skin_changer", "skin changer", 230.0f, []
			{
				toggle("active", &cfg::skins_active);
				sliderFloat("apply delay", &cfg::skins_apply_delay, 0.0f, 2.0f, "%.1fs");

				if (!customWidget("apply now")) {
					return;
				}

				if (ImGui::Button("apply now")) {
					skin_changer->applyNow();
				}

				ImGui::SameLine();

				if (ImGui::Button("clear all")) {
					confirm("remove every weapon override?", [] { skin_changer->clear(); });
				}

				ImGui::SameLine();

				if (ImGui::Button("dump"))
				{
					const std::string path{ utils::getFilePath() + "skin_catalog.txt" };

					std::ofstream{ path } << skin_catalog->dump();
					notifs->message(std::format("wrote {}", path));
				}
			});

			section("skins_weapons", "weapons", 230.0f, []
			{
				//search walks this page without a game loaded, the list is only worth drawing for real
				if (!customWidget("weapon list", &cfg::skins_overrides) || widgetMode() != WidgetMode::DRAW) {
					return;
				}

				if (!skin_catalog->ensure())
				{
					ImGui::TextDisabled("waiting for the item schema");
					return;
				}

				weaponList(skin_changer->equippedFamilies());
			});
		}
		endColumn();

		nextColumn();

		beginColumn("group_skins1");
		{
			section("skins_override", "override", 270.0f, []
			{
				if (!customWidget("weapon override", &cfg::skins_overrides) || widgetMode() != WidgetMode::DRAW) {
					return;
				}

				if (!skin_catalog->ensure()) {
					return;
				}

				const std::vector<int> equipped{ skin_changer->equippedFamilies() };

				//start on the weapon in your hands
				if (selected_base < 0 && !equipped.empty()) {
					selected_base = equipped.front();
				}

				overrideEditor(equipped);
			}, 150.0f);
		}
		endColumn();
	}
	endPage();
}
