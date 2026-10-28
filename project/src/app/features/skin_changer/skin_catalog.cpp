#include "skin_catalog.hpp"
#include "equip_regions.hpp"

#include "../statistics/statistics.hpp"

#include <optional>
#include <set>

//the definition keeps its prefab merged keyvalues right after the vtable
constexpr size_t ITEM_DEF_KV_OFFSET{ 8 };

//the schema's own reload counter, game code caches attribute lookups on it too
constexpr size_t SCHEMA_VERSION_OFFSET{ 8 };

//anything past this isn't a real item, GetItemDefinition hands back its shared fallback for it
constexpr int INVALID_DEF{ 65535 };

//attributes that only change the look, two items that differ in nothing else are the same weapon
static const std::set<std::string> cosmetic_attributes
{
	"paintkit_proto_def_index", "set_item_texture_wear", "custom_paintkit_seed_lo", "custom_paintkit_seed_hi",
	"has team color paintkit", "texture_wear_default", "weapon_uses_stattrak_module", "weapon_stattrak_module_scale",
	"weapon_allow_inspect", "is australium item", "item style override", "is_festivized", "loot rarity",
	"kill eater", "kill eater score type", "kill eater 2", "kill eater score type 2", "kill eater 3", "kill eater score type 3",
	"turn to gold", "disable fancy class select anim", "min_viewmodel_offset", "inspect_viewmodel_offset",
	"attach particle effect", "attach particle effect static", "killstreak tier", "killstreak effect", "killstreak idleeffect",
	"limited quantity item", "cannot trade", "always tradable", "never craftable", "unique craft index",
	"custom employee number", "tool target item", "is commodity", "set supply crate series", "series number",
	"gifter account id", "event date", "cannot restore", "is marketable", "item name text override"
};

static const std::pair<const char *, int> class_names[]
{
	{ "scout", TF_CLASS_SCOUT }, { "sniper", TF_CLASS_SNIPER }, { "soldier", TF_CLASS_SOLDIER },
	{ "demoman", TF_CLASS_DEMOMAN }, { "medic", TF_CLASS_MEDIC }, { "heavy", TF_CLASS_HEAVYWEAPONS },
	{ "pyro", TF_CLASS_PYRO }, { "spy", TF_CLASS_SPY }, { "engineer", TF_CLASS_ENGINEER }
};

static std::string lower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });

	return str;
}

//numbers in the schema get parsed into ints and floats, everything else stays a string
static std::string kvValue(KeyValues *const kv)
{
	if (!kv) {
		return {};
	}

	switch (kv->m_iDataType)
	{
		case TYPE_STRING: {
			return kv->m_sValue ? kv->m_sValue : "";
		}

		case TYPE_INT: {
			return std::to_string(kv->m_iValue);
		}

		case TYPE_FLOAT: {
			return std::format("{}", kv->m_flValue);
		}

		case TYPE_WSTRING: {
			return kv->m_wsValue ? utils::wideToUtf8(kv->m_wsValue) : "";
		}

		default: {
			return {};
		}
	}
}

static KeyValues *child(KeyValues *const kv, const char *const name)
{
	if (!kv) {
		return nullptr;
	}

	for (KeyValues *sub{ kv->m_pSub }; sub; sub = sub->m_pPeer)
	{
		const char *const sub_name{ sub->GetName() };

		if (sub_name && _stricmp(sub_name, name) == 0) {
			return sub;
		}
	}

	return nullptr;
}

static std::string childValue(KeyValues *const kv, const char *const name)
{
	return kvValue(child(kv, name));
}

//"TF_WEAPON_ROCKETLAUNCHER" -> "Rocketlauncher", "Upgradeable TF_WEAPON_X" -> "X", "The Ambassador" -> "Ambassador"
static std::string tidyName(std::string name)
{
	for (const char *const prefix : { "The ", "Upgradeable " })
	{
		if (name.starts_with(prefix)) {
			name.erase(0, strlen(prefix));
		}
	}

	if (name.starts_with("TF_WEAPON_"))
	{
		name = lower(name.substr(10));
		std::replace(name.begin(), name.end(), '_', ' ');

		if (!name.empty()) {
			name.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(name.front())));
		}
	}

	return name;
}

static std::string itemName(const int def, KeyValues *const kv)
{
	if (const char *const name{ weaponName(def) }) {
		return std::string{ name }.starts_with("The ") ? std::string{ name }.substr(4) : name;
	}

	return tidyName(childValue(kv, "name"));
}

//what the gameplay side of the item boils down to, the cosmetic attributes left out
static std::string gameplaySignature(KeyValues *const kv)
{
	std::vector<std::string> parts{};

	//"attributes" { "name" { "attribute_class" ... "value" ... } }
	if (KeyValues *const attributes{ child(kv, "attributes") })
	{
		for (KeyValues *attr{ attributes->m_pSub }; attr; attr = attr->m_pPeer)
		{
			const std::string name{ lower(attr->GetName() ? attr->GetName() : "") };

			if (!name.empty() && !cosmetic_attributes.contains(name)) {
				parts.push_back(name + "=" + childValue(attr, "value"));
			}
		}
	}

	//"static_attrs" { "name" "value" }
	if (KeyValues *const static_attrs{ child(kv, "static_attrs") })
	{
		for (KeyValues *attr{ static_attrs->m_pSub }; attr; attr = attr->m_pPeer)
		{
			const std::string name{ lower(attr->GetName() ? attr->GetName() : "") };

			if (!name.empty() && !cosmetic_attributes.contains(name)) {
				parts.push_back(name + "=" + kvValue(attr));
			}
		}
	}

	std::sort(parts.begin(), parts.end());

	std::string out{};

	for (const std::string &part : parts) {
		out += part + ";";
	}

	return out;
}

static bool hasAttribute(KeyValues *const kv, const char *const name)
{
	return child(child(kv, "attributes"), name) || child(child(kv, "static_attrs"), name);
}

std::string localize(std::string_view token)
{
	//the game's own string table. reading resource/tf_english.txt ourselves found nothing (it's utf-16), which left every
	//unusual effect nameless and the cosmetic names on their internal fallbacks
	if (!i::localize || token.empty()) {
		return "";
	}

	const wchar_t *const text{ i::localize->Find(std::string{ token }.c_str()) };

	if (!text || !*text) {
		return "";
	}

	const int size{ WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr) };

	if (size <= 1) {
		return "";
	}

	std::string out(static_cast<size_t>(size - 1), '\0');

	WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), size, nullptr, nullptr);

	return out;
}

//"#Token" to english, or the text itself when it isn't a token
static std::string localizedOr(const std::string &text, const std::string &fallback)
{
	if (!text.starts_with('#')) {
		return text.empty() ? fallback : text;
	}

	const std::string out{ localize(text) };

	return out.empty() ? fallback : out;
}

static std::vector<SkinStyle> readStyles(KeyValues *const visuals, bool *const australium)
{
	std::vector<SkinStyle> out{};

	KeyValues *const styles{ child(visuals, "styles") };

	if (!styles) {
		return out;
	}

	for (KeyValues *style{ styles->m_pSub }; style; style = style->m_pPeer)
	{
		const char *const index{ style->GetName() };

		if (!index || !isdigit(static_cast<unsigned char>(*index))) {
			continue;
		}

		SkinStyle entry{ atoi(index), "" };

		entry.name = localizedOr(childValue(style, "name"), std::format("style {}", entry.index));

		//the australium look is a gold skin on the normal model
		if (const std::string icon{ lower(childValue(style, "image_inventory")) }; australium && icon.ends_with("_gold"))
		{
			*australium = true;
			entry.name = "australium";
		}

		out.push_back(entry);
	}

	std::sort(out.begin(), out.end(), [](const SkinStyle &a, const SkinStyle &b) { return a.index < b.index; });

	return out;
}

static SkinItem readItem(const int def, KeyValues *const kv)
{
	SkinItem item{};

	item.def = def;
	item.name = itemName(def, kv);
	item.item_class = childValue(kv, "item_class");
	item.slot = lower(childValue(kv, "item_slot"));

	const std::string internal_name{ childValue(kv, "name") };

	item.stock = childValue(kv, "baseitem") == "1";
	item.upgradeable = internal_name.starts_with("Upgradeable ");
	item.festive = internal_name.find("Festive") != std::string::npos;
	item.decorated = (def >= 15000 && def < 16000) || childValue(child(kv, "tags"), "can_apply_paintkit") == "1";

	if (KeyValues *const used_by{ child(kv, "used_by_classes") })
	{
		for (const auto &[name, id] : class_names)
		{
			if (child(used_by, name)) {
				item.classes |= 1u << id;
			}
		}
	}

	//all class melees don't list anyone
	if (!item.classes)
	{
		for (const auto &[name, id] : class_names) {
			item.classes |= 1u << id;
		}
	}

	KeyValues *const visuals{ child(kv, "visuals") };

	//quick-fix and vaccinator keep their visuals per team
	const bool festive_models{ child(visuals, "attached_models_festive") || child(child(kv, "visuals_red"), "attached_models_festive") };

	item.festivizable = festive_models && childValue(child(kv, "tags"), "can_be_festivized") == "1";

	item.styles = readStyles(visuals, &item.australium);

	if (CUtlVector<int> *const kits{ tf_utils::getValidPaintKits(def) })
	{
		for (int n{}; n < kits->Count(); n++) {
			item.paintkits.push_back((*kits)[n]);
		}
	}

	item.stattrak = !item.paintkits.empty() || hasAttribute(kv, "weapon_uses_stattrak_module");

	item.extra_wearable = childValue(kv, "extra_wearable");
	item.extra_wearable_vm = childValue(kv, "extra_wearable_vm");
	item.botkiller = !item.extra_wearable.empty() && internal_name.find("Botkiller") != std::string::npos;

	if (const std::string skin{ childValue(child(kv, "visuals_red"), "skin") }; !skin.empty()) {
		item.skin_red = atoi(skin.c_str());
	}

	if (const std::string skin{ childValue(child(kv, "visuals_blu"), "skin") }; !skin.empty()) {
		item.skin_blu = atoi(skin.c_str());
	}

	return item;
}

//"equip_region" "hat", "equip_region" { "hat" "1" }, "equip_regions" { ... }, an item can use both keys
static void readRegions(KeyValues *const kv, std::vector<std::string> &out)
{
	for (const char *const key : { "equip_region", "equip_regions" })
	{
		KeyValues *const region{ child(kv, key) };

		if (!region) {
			continue;
		}

		if (!region->m_pSub)
		{
			if (const std::string name{ lower(kvValue(region)) }; !name.empty()) {
				out.push_back(name);
			}

			continue;
		}

		for (KeyValues *sub{ region->m_pSub }; sub; sub = sub->m_pPeer)
		{
			if (const char *const name{ sub->GetName() }) {
				out.push_back(lower(name));
			}
		}
	}

	std::sort(out.begin(), out.end());
	out.erase(std::unique(out.begin(), out.end()), out.end());
}

//nullopt for anything that isn't a cosmetic you could put in a loadout slot
static std::optional<SkinCosmetic> readCosmetic(const int def, KeyValues *const kv)
{
	const std::string slot{ lower(childValue(kv, "item_slot")) };

	if (slot != "head" && slot != "misc") {
		return std::nullopt;
	}

	//test items, medals, zombie souls and promo duplicates
	if (childValue(kv, "hidden") == "1" || childValue(kv, "show_in_armory") == "0") {
		return std::nullopt;
	}

	SkinCosmetic out{};

	out.def = def;
	out.slot = slot;

	if (KeyValues *const used_by{ child(kv, "used_by_classes") })
	{
		for (const auto &[name, id] : class_names)
		{
			if (child(used_by, name)) {
				out.classes |= 1u << id;
			}
		}
	}

	if (!out.classes) {
		return std::nullopt;
	}

	out.name = localizedOr(childValue(kv, "item_name"), tidyName(childValue(kv, "name")));

	readRegions(kv, out.regions);

	for (const std::string &region : out.regions)
	{
		if (const int bit{ equip_regions::bitOf(region) }; bit >= 0) {
			out.own |= 1u << bit;
		}
	}

	out.mask = equip_regions::expand(out.own);

	out.paintable = childValue(child(kv, "capabilities"), "paintable") == "1";

	KeyValues *const visuals{ child(kv, "visuals") };

	out.styles = readStyles(visuals, nullptr);

	//a 1 hides that part of the player model, a 0 only undoes what a prefab hid
	if (KeyValues *const bodygroups{ child(visuals, "player_bodygroups") })
	{
		for (KeyValues *group{ bodygroups->m_pSub }; group; group = group->m_pPeer)
		{
			if (kvValue(group) == "1" && group->GetName())
			{
				out.hides_bodygroups = true;
				out.bodygroups.emplace_back(group->GetName());
			}
		}
	}

	return out;
}

void SkinCatalog::build(void *const schema)
{
	m_items.clear();
	m_families.clear();
	m_item_index.clear();
	m_cosmetics.clear();
	m_cosmetic_index.clear();

	void *const fallback{ s::CEconItemSchema_GetItemDefinition.call<void *>(schema, INVALID_DEF) };

	//what decides which items are one weapon, kept per family for the second merge pass
	std::unordered_map<std::string, size_t> family_by_key{};
	std::vector<std::string> family_signature{};

	std::vector<std::pair<SkinItem, std::string>> pending{}; //item, family key

	for (int def{}; def < INVALID_DEF; def++)
	{
		void *const item_def{ s::CEconItemSchema_GetItemDefinition.call<void *>(schema, def) };

		if (!item_def || item_def == fallback) {
			continue;
		}

		KeyValues *const kv{ *reinterpret_cast<KeyValues **>(reinterpret_cast<uintptr_t>(item_def) + ITEM_DEF_KV_OFFSET) };

		if (!kv) {
			continue;
		}

		const std::string item_class{ childValue(kv, "item_class") };

		//wearables in a weapon slot (shields, boots, razorback) fall out on their item_slot
		if (item_class.starts_with("tf_wearable"))
		{
			if (std::optional<SkinCosmetic> cosmetic{ readCosmetic(def, kv) }) {
				m_cosmetics.push_back(std::move(*cosmetic));
			}

			continue;
		}

		if (!item_class.starts_with("tf_weapon_") && item_class != "saxxy") {
			continue;
		}

		SkinItem item{ readItem(def, kv) };

		//the base name ties the class specific stock copies together (shotguns, pistols), the item class is the fallback
		std::string key{ lower(childValue(kv, "base_item_name")) };

		if (key.empty()) {
			key = item_class;
		}

		//all class melees share one item class and have no stats of their own, the model is what tells them apart
		if (item_class == "saxxy") {
			key += "|" + lower(childValue(kv, "model_player"));
		}

		key += "|" + gameplaySignature(kv);

		pending.emplace_back(std::move(item), std::move(key));
	}

	//first pass, by key
	std::vector<std::vector<size_t>> groups{};

	for (size_t n{}; n < pending.size(); n++)
	{
		const std::string &key{ pending[n].second };

		auto it{ family_by_key.find(key) };

		if (it == family_by_key.end())
		{
			it = family_by_key.emplace(key, groups.size()).first;
			groups.emplace_back();
			family_signature.push_back(key.substr(key.rfind('|') + 1));
		}

		groups[it->second].push_back(n);
	}

	//the base item of a group: the free stock one, else the lowest non decorated def
	auto pickBase = [&](const std::vector<size_t> &group) -> size_t
	{
		size_t best{ group.front() };

		for (const size_t n : group)
		{
			const SkinItem &a{ pending[n].first };
			const SkinItem &b{ pending[best].first };

			if (a.stock != b.stock) {
				best = a.stock ? n : best;
				continue;
			}

			if (a.decorated != b.decorated) {
				best = !a.decorated ? n : best;
				continue;
			}

			if (a.def < b.def) {
				best = n;
			}
		}

		return best;
	};

	//second pass, the same weapon under two item classes (engineer's shotgun vs the all class upgradeable one) has the same name and stats
	std::unordered_map<std::string, size_t> merged_by_name{};
	std::vector<std::vector<size_t>> merged{};

	for (size_t g{}; g < groups.size(); g++)
	{
		const std::string name_key{ lower(pending[pickBase(groups[g])].first.name) + "|" + family_signature[g] };

		auto it{ merged_by_name.find(name_key) };

		if (it == merged_by_name.end()) {
			it = merged_by_name.emplace(name_key, merged.size()).first;
			merged.emplace_back();
		}

		merged[it->second].insert(merged[it->second].end(), groups[g].begin(), groups[g].end());
	}

	for (std::vector<size_t> &group : merged)
	{
		std::sort(group.begin(), group.end(), [&](const size_t a, const size_t b) { return pending[a].first.def < pending[b].first.def; });

		const SkinItem &base{ pending[pickBase(group)].first };

		SkinFamily family{};

		family.base = base.def;
		family.name = base.name;
		family.slot = base.slot;

		const int family_index{ static_cast<int>(m_families.size()) };

		//variants named the same as the base get told apart by what they are
		for (const size_t n : group)
		{
			SkinItem item{ pending[n].first };

			if (item.def != base.def && item.name == base.name) {
				item.name += item.upgradeable ? " (upgradeable)" : std::format(" ({})", item.def);
			}

			else if (item.def == base.def && item.stock && group.size() > 1) {
				item.name += " (stock)";
			}

			item.family = family_index;

			family.classes |= item.classes;
			family.any_australium |= item.australium;
			family.any_festivizable |= item.festivizable;
			family.any_paintkits |= !item.paintkits.empty();
			family.any_botkiller |= item.botkiller;
			family.members.push_back(item.def);

			m_item_index[item.def] = m_items.size();
			m_items.push_back(std::move(item));
		}

		m_families.push_back(std::move(family));
	}

	//fix the family names up after the members got their suffixes
	for (SkinFamily &family : m_families)
	{
		if (const char *const name{ weaponName(family.base) }) {
			family.name = std::string{ name }.starts_with("The ") ? std::string{ name }.substr(4) : name;
		}
	}

	std::sort(m_families.begin(), m_families.end(), [](const SkinFamily &a, const SkinFamily &b) { return a.name < b.name; });

	//sorting moved them, point the items back at their family
	for (int n{}; n < static_cast<int>(m_families.size()); n++)
	{
		for (const int def : m_families[n].members) {
			m_items[m_item_index[def]].family = n;
		}
	}

	std::sort(m_cosmetics.begin(), m_cosmetics.end(), [](const SkinCosmetic &a, const SkinCosmetic &b) { return a.name < b.name; });

	for (size_t n{}; n < m_cosmetics.size(); n++) {
		m_cosmetic_index[m_cosmetics[n].def] = n;
	}

	buildUnusuals();
}

//items_game "attribute_controlled_attached_particles", red id -> blu id. only the red one has a name
static const std::pair<int, int> team_unusuals[]
{
	{ 59, 55 }, { 139, 140 }, { 145, 146 }, { 175, 176 }, { 183, 184 }, { 193, 194 }, { 207, 208 }, { 216, 217 },
	{ 221, 222 }, { 224, 225 }, { 226, 227 }, { 239, 240 }, { 245, 246 }, { 261, 262 }, { 267, 268 }, { 274, 275 },
	{ 279, 280 }, { 281, 282 }, { 295, 296 }, { 297, 298 }, { 301, 302 }, { 309, 310 }, { 312, 313 }, { 315, 316 },
	{ 318, 319 }, { 322, 323 }, { 327, 328 }, { 331, 332 }, { 337, 338 }, { 342, 343 }, { 346, 347 }, { 360, 361 },
	{ 362, 363 }, { 365, 366 }, { 374, 375 }, { 377, 378 }, { 382, 383 }, { 385, 386 }, { 387, 388 }, { 390, 391 },
	{ 397, 398 }, { 399, 400 }, { 410, 411 }, { 413, 414 }, { 416, 417 }, { 421, 422 }
};

//the cosmetic effects are 6 and up, weapon ones start at 701
constexpr int COSMETIC_UNUSUAL_FIRST{ 6 };
constexpr int COSMETIC_UNUSUAL_LAST{ 699 };

void SkinCatalog::buildUnusuals()
{
	m_unusuals.clear();

	for (int id{ COSMETIC_UNUSUAL_FIRST }; id <= COSMETIC_UNUSUAL_LAST; id++)
	{
		if (std::any_of(std::begin(team_unusuals), std::end(team_unusuals), [&](const auto &pair) { return pair.second == id; })) {
			continue;
		}

		std::string name{ localize(std::format("Attrib_Particle{}", id)) };

		if (!name.empty()) {
			m_unusuals.push_back(SkinUnusual{ id, std::move(name) });
		}
	}

	std::sort(m_unusuals.begin(), m_unusuals.end(), [](const SkinUnusual &a, const SkinUnusual &b) { return a.name < b.name; });
}

int SkinCatalog::unusualForTeam(const int id, const int team)
{
	if (team != TF_TEAM_BLU) {
		return id;
	}

	for (const auto &[red, blu] : team_unusuals)
	{
		if (red == id) {
			return blu;
		}
	}

	return id;
}

//items_game paint cans, "set item tint RGB" / "set item tint RGB 2"
const std::vector<SkinPaint> &SkinCatalog::paints()
{
	static const std::vector<SkinPaint> list
	{
		{ "Indubitably Green", 7511618 }, { "Zepheniah's Greed", 4345659 }, { "Noble Hatter's Violet", 5322826 },
		{ "Color No. 216-190-216", 14204632 }, { "A Deep Commitment to Purple", 8208497 }, { "Mann Co. Orange", 13595446 },
		{ "Muskelmannbraun", 10843461 }, { "Peculiarly Drab Tincture", 12955537 }, { "Radigan Conagher Brown", 6901050 },
		{ "Ye Olde Rustic Colour", 8154199 }, { "Australium Gold", 15185211 }, { "Aged Moustache Grey", 8289918 },
		{ "An Extraordinary Abundance of Tinge", 15132390 }, { "A Distinctive Lack of Hue", 1315860 }, { "Pink as Hell", 16738740 },
		{ "A Color Similar to Slate", 3100495 }, { "Drably Olive", 8421376 }, { "The Bitter Taste of Defeat and Lime", 3329330 },
		{ "The Color of a Gentlemann's Business Pants", 15787660 }, { "Dark Salmon Injustice", 15308410 }, { "A Mann's Mint", 12377523 },
		{ "After Eight", 2960676 },

		{ "Team Spirit", 12073019, 5801378 }, { "Operator's Overalls", 4732984, 3686984 }, { "Waterlogged Lab Coat", 11049612, 8626083 },
		{ "Balaclavas Are Forever", 3874595, 1581885 }, { "An Air of Debonair", 6637376, 2636109 }, { "The Value of Teamwork", 8400928, 2452877 },
		{ "Cream Spirit", 12807213, 12091445 }
	};

	return list;
}

const SkinCosmetic *SkinCatalog::cosmetic(const int def) const
{
	const auto it{ m_cosmetic_index.find(def) };

	return it == m_cosmetic_index.end() ? nullptr : &m_cosmetics[it->second];
}

std::string SkinCatalog::conflict(const SkinCosmetic &a, const SkinCosmetic &b) const
{
	if (!(a.mask & b.own) && !(b.mask & a.own))
	{
		//a region the table doesn't know still clashes with itself
		for (const std::string &region : a.regions)
		{
			if (equip_regions::bitOf(region) < 0 && std::find(b.regions.begin(), b.regions.end(), region) != b.regions.end()) {
				return region;
			}
		}

		return "";
	}

	//name the region of a that's in the way
	for (const std::string &region : a.regions)
	{
		const int bit{ equip_regions::bitOf(region) };

		if (bit >= 0 && (((1u << bit) & b.mask) || (equip_regions::expand(1u << bit) & b.own))) {
			return region;
		}
	}

	return a.regions.empty() ? "equip region" : a.regions.front();
}

bool SkinCatalog::ensure()
{
	void *const schema{ s::GetItemSchema.call<void *>() };

	if (!schema) {
		return false;
	}

	const int version{ *reinterpret_cast<int *>(reinterpret_cast<uintptr_t>(schema) + SCHEMA_VERSION_OFFSET) };

	//an empty result means the schema wasn't loaded yet, try again every few seconds
	static float next_retry{};

	const bool retry{ m_families.empty() && i::global_vars->realtime >= next_retry };

	if (version != m_schema_version || retry)
	{
		m_schema_version = version;
		next_retry = i::global_vars->realtime + 5.0f;

		build(schema);
	}

	return !m_families.empty();
}

const SkinItem *SkinCatalog::item(const int def) const
{
	const auto it{ m_item_index.find(def) };

	return it == m_item_index.end() ? nullptr : &m_items[it->second];
}

const SkinFamily *SkinCatalog::familyOf(const int def) const
{
	const SkinItem *const found{ item(def) };

	return found ? &m_families[found->family] : nullptr;
}

const SkinFamily *SkinCatalog::familyByBase(const int base) const
{
	const SkinFamily *const family{ familyOf(base) };

	return family && family->base == base ? family : nullptr;
}

std::string SkinCatalog::dump() const
{
	std::string out{ std::format("{} families, {} items\n\n", m_families.size(), m_items.size()) };

	for (const SkinFamily &family : m_families)
	{
		out += std::format("{} (base {}, slot {}, classes 0x{:X}){}{}{}{}\n", family.name, family.base, family.slot, family.classes,
			family.any_australium ? " [australium]" : "", family.any_festivizable ? " [festivizable]" : "",
			family.any_paintkits ? " [paintkits]" : "", family.any_botkiller ? " [botkiller]" : "");

		for (const int def : family.members)
		{
			const SkinItem *const it{ item(def) };

			if (!it) {
				continue;
			}

			out += std::format("    {:>5} {} [{}]{}{}{}{}{}{}{}{} styles {} kits {}\n", it->def, it->name, it->item_class,
				it->stock ? " stock" : "", it->upgradeable ? " upgradeable" : "", it->festive ? " festive" : "",
				it->decorated ? " decorated" : "", it->botkiller ? " botkiller" : "", it->festivizable ? " festivizable" : "",
				it->australium ? " australium" : "", it->stattrak ? " stattrak" : "", it->styles.size(), it->paintkits.size());
		}
	}

	out += std::format("\n{} cosmetics, {} unusual effects\n\n", m_cosmetics.size(), m_unusuals.size());

	for (const SkinCosmetic &cosmetic : m_cosmetics)
	{
		std::string regions{};

		for (const std::string &region : cosmetic.regions) {
			regions += (regions.empty() ? "" : ",") + region;
		}

		out += std::format("{:>6} {} [{}] classes 0x{:X} regions {} (own 0x{:X} mask 0x{:X}){}{} styles {}\n", cosmetic.def, cosmetic.name, cosmetic.slot,
			cosmetic.classes, regions.empty() ? "none" : regions, cosmetic.own, cosmetic.mask,
			cosmetic.paintable ? " paintable" : "", cosmetic.hides_bodygroups ? " bodygroups" : "", cosmetic.styles.size());
	}

	return out;
}
