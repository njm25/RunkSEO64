#include "skin_changer.hpp"

#include "../cfg.hpp"

#include <bit>
#include <unordered_set>

//kRenderNone, C_BaseEntity::ShouldDraw draws nothing in it
constexpr unsigned char RENDER_MODE_NONE{ 10 };

static C_BaseEntity *fromHandle(const unsigned long handle);
static unsigned long handleOf(C_BaseEntity *const ent);

//everything that changes what the weapons and wearables get built with
static std::string settingsKey()
{
	return cfg::skins_active ? cfg::skins_overrides + "|" + cfg::skins_player : "";
}

//the four effects weapons can have, in the order the menu lists them
static float unusualEffect(const int option)
{
	constexpr int effects[]{ 0, weapon_unusual_effects::weapon_unusual_hot, weapon_unusual_effects::weapon_unusual_isotope, weapon_unusual_effects::weapon_unusual_cool, weapon_unusual_effects::weapon_unusual_energyorb };

	return static_cast<float>(effects[std::clamp(option, 0, 4)]);
}

static float wearValue(const int wear)
{
	//factory new to battle scarred
	return 0.2f + 0.2f * static_cast<float>(std::clamp(wear, 0, 4));
}

//setting an attribute marks the item's attribute cache dirty, so only when it's actually different
static void setAttribute(CAttributeList *const list, const uint16_t index, const float value)
{
	for (int n{}; n < list->m_Attributes.Count(); n++)
	{
		const CEconItemAttribute &attr{ list->m_Attributes[n] };

		//the game's index is 16 bit, the 2 bytes after it are padding
		if (static_cast<uint16_t>(attr.m_iAttributeDefinitionIndex) == index && attr.m_flValue == value) {
			return;
		}
	}

	list->SetAttribute(index, value);
}

static std::string familyKey(const int base)
{
	return std::format("#{}", base);
}

static nlohmann::json toJson(const WeaponOverride &o)
{
	return {
		{ "enabled", o.enabled }, { "variant", o.variant }, { "paintkit", o.paintkit }, { "wear", o.wear }, { "seed", o.seed },
		{ "australium", o.australium }, { "festivized", o.festivized }, { "stattrak", o.stattrak },
		{ "ks_tier", o.ks_tier }, { "sheen", o.sheen }, { "killstreaker", o.killstreaker },
		{ "unusual", o.unusual }, { "style", o.style }, { "botkiller", o.botkiller },
		{ "spell_pumpkin", o.spell_pumpkin }, { "spell_green_flames", o.spell_green_flames }
	};
}

static WeaponOverride fromJson(const nlohmann::json &j)
{
	WeaponOverride o{};

	o.enabled = j.value("enabled", o.enabled);
	o.variant = j.value("variant", o.variant);
	o.paintkit = j.value("paintkit", o.paintkit);
	o.wear = j.value("wear", o.wear);
	o.seed = j.value("seed", o.seed);
	o.australium = j.value("australium", o.australium);
	o.festivized = j.value("festivized", o.festivized);
	o.stattrak = j.value("stattrak", o.stattrak);
	o.ks_tier = j.value("ks_tier", o.ks_tier);
	o.sheen = j.value("sheen", o.sheen);
	o.killstreaker = j.value("killstreaker", o.killstreaker);
	o.unusual = j.value("unusual", o.unusual);
	o.style = j.value("style", o.style);
	o.botkiller = j.value("botkiller", o.botkiller);
	o.spell_pumpkin = j.value("spell_pumpkin", o.spell_pumpkin);
	o.spell_green_flames = j.value("spell_green_flames", o.spell_green_flames);

	return o;
}

void SkinChanger::parse()
{
	if (m_parsed_from == cfg::skins_overrides) {
		return;
	}

	m_parsed_from = cfg::skins_overrides;
	m_overrides.clear();

	if (cfg::skins_overrides.empty()) {
		return;
	}

	//a broken string (hand edited config) just means no overrides
	const nlohmann::json j{ nlohmann::json::parse(cfg::skins_overrides, nullptr, false) };

	if (!j.is_object()) {
		return;
	}

	for (const auto &[key, value] : j.items())
	{
		if (key.size() < 2 || key.front() != '#' || !value.is_object()) {
			continue;
		}

		const int base{ atoi(key.c_str() + 1) };

		if (base > 0 || key == "#0") {
			m_overrides[base] = fromJson(value);
		}
	}
}

const std::unordered_map<int, WeaponOverride> &SkinChanger::overrides()
{
	parse();

	return m_overrides;
}

const WeaponOverride *SkinChanger::find(const int base)
{
	parse();

	const auto it{ m_overrides.find(base) };

	return it == m_overrides.end() ? nullptr : &it->second;
}

//sorted by def so the saved string (and so the rebuild check) doesn't change with map order
static std::string serialize(const std::unordered_map<int, WeaponOverride> &overrides)
{
	if (overrides.empty()) {
		return "";
	}

	std::vector<int> bases{};

	for (const auto &[base, _] : overrides) {
		bases.push_back(base);
	}

	std::sort(bases.begin(), bases.end());

	nlohmann::ordered_json j = nlohmann::ordered_json::object();

	for (const int base : bases) {
		j[familyKey(base)] = toJson(overrides.at(base));
	}

	return j.dump();
}

void SkinChanger::set(const int base, const WeaponOverride &override)
{
	parse();

	m_overrides[base] = override;

	cfg::skins_overrides = serialize(m_overrides);
	m_parsed_from = cfg::skins_overrides;
}

void SkinChanger::erase(const int base)
{
	parse();

	m_overrides.erase(base);

	cfg::skins_overrides = serialize(m_overrides);
	m_parsed_from = cfg::skins_overrides;
}

void SkinChanger::clear()
{
	m_overrides.clear();

	cfg::skins_overrides.clear();
	m_parsed_from.clear();
}

std::string SkinChanger::unsupported(const SkinItem &item, const WeaponOverride &o) const
{
	if (o.paintkit && std::find(item.paintkits.begin(), item.paintkits.end(), o.paintkit) == item.paintkits.end()) {
		return "no war paints";
	}

	if (o.australium && !item.australium) {
		return "no australium";
	}

	if (o.festivized && !item.festivizable) {
		return "can't be festivized";
	}

	if (o.stattrak && !item.stattrak) {
		return "no strange counter";
	}

	if (o.style >= 0 && std::none_of(item.styles.begin(), item.styles.end(), [&](const SkinStyle &s) { return s.index == o.style; })) {
		return "no such style";
	}

	return "";
}

int SkinChanger::resolveVariant(const SkinFamily &family, const WeaponOverride &o, const int current_def) const
{
	//picked by hand, as long as it's still one of the family
	if (o.variant && std::find(family.members.begin(), family.members.end(), o.variant) != family.members.end()) {
		return o.variant;
	}

	const SkinItem *const current{ skin_catalog->item(current_def) };

	if (current && unsupported(*current, o).empty()) {
		return current_def;
	}

	//the upgradeable copy is what strange/australium/war paints go on, then anything plain, the special looks last
	auto rank = [](const SkinItem &item)
	{
		if (item.upgradeable) {
			return 0;
		}

		if (!item.festive && !item.botkiller && !item.decorated) {
			return 1;
		}

		return item.decorated ? 3 : 2;
	};

	int best{ current_def };
	int best_rank{ INT_MAX };

	for (const int def : family.members)
	{
		const SkinItem *const item{ skin_catalog->item(def) };

		if (!item || !unsupported(*item, o).empty()) {
			continue;
		}

		if (const int r{ rank(*item) }; r < best_rank) {
			best = def;
			best_rank = r;
		}
	}

	return best;
}

std::vector<int> SkinChanger::equippedFamilies() const
{
	std::vector<int> out{};

	C_TFPlayer *const local{ i::engine->IsInGame() ? ec->getLocal() : nullptr };

	if (!local || !skin_catalog->ensure()) {
		return out;
	}

	for (int n{}; n < MAX_WEAPONS; n++)
	{
		C_TFWeaponBase *const weapon{ local->GetWeapon(n) };

		if (!weapon) {
			continue;
		}

		if (const SkinFamily *const family{ skin_catalog->familyOf(weapon->m_iItemDefinitionIndex()) }) {
			out.push_back(family->base);
		}
	}

	return out;
}

static const std::pair<const char *, int> player_classes[]
{
	{ "scout", TF_CLASS_SCOUT }, { "soldier", TF_CLASS_SOLDIER }, { "pyro", TF_CLASS_PYRO },
	{ "demoman", TF_CLASS_DEMOMAN }, { "heavy", TF_CLASS_HEAVYWEAPONS }, { "engineer", TF_CLASS_ENGINEER },
	{ "medic", TF_CLASS_MEDIC }, { "sniper", TF_CLASS_SNIPER }, { "spy", TF_CLASS_SPY }
};

static bool validClass(const int cls)
{
	return cls > TF_CLASS_UNDEFINED && cls < TF_CLASS_CIVILIAN;
}

void SkinChanger::parsePlayer()
{
	if (m_player_parsed_from == cfg::skins_player) {
		return;
	}

	m_player_parsed_from = cfg::skins_player;
	m_player = {};

	const nlohmann::json j{ nlohmann::json::parse(cfg::skins_player, nullptr, false) };

	if (!j.is_object()) {
		return;
	}

	for (const auto &[name, cls] : player_classes)
	{
		const auto it{ j.find(name) };

		if (it == j.end() || !it->is_object()) {
			continue;
		}

		ClassOverride &o{ m_player[cls] };

		o.enabled = it->value("enabled", false);

		if (const auto slots{ it->find("slots") }; slots != it->end() && slots->is_array())
		{
			for (size_t n{}; n < std::min(slots->size(), o.slots.size()); n++)
			{
				const nlohmann::json &slot{ (*slots)[n] };

				if (!slot.is_object()) {
					continue;
				}

				o.slots[n].def = slot.value("def", 0);
				o.slots[n].paint = slot.value("paint", 0);
				o.slots[n].style = slot.value("style", -1);
				o.slots[n].unusual = slot.value("unusual", 0);
			}
		}
	}
}

const ClassOverride &SkinChanger::classOverride(const int cls)
{
	parsePlayer();

	static const ClassOverride none{};

	return validClass(cls) ? m_player[cls] : none;
}

std::string SkinChanger::slotConflict(const int cls, const int slot, const int def)
{
	const SkinCosmetic *const item{ skin_catalog->cosmetic(def) };

	if (!item || !validClass(cls)) {
		return "";
	}

	const ClassOverride &o{ classOverride(cls) };

	for (int n{}; n < static_cast<int>(o.slots.size()); n++)
	{
		const SkinCosmetic *const other{ n != slot ? skin_catalog->cosmetic(o.slots[n].def) : nullptr };

		if (!other) {
			continue;
		}

		if (const std::string region{ skin_catalog->conflict(*item, *other) }; !region.empty()) {
			return std::format("conflicts with {} ({})", other->name, region);
		}
	}

	return "";
}

void SkinChanger::setClassOverride(const int cls, ClassOverride override)
{
	if (!validClass(cls)) {
		return;
	}

	parsePlayer();

	//the loadout's own rules: only what the class can wear, and nothing that clashes with a slot before it
	for (size_t n{}; n < override.slots.size(); n++)
	{
		CosmeticSlot &slot{ override.slots[n] };
		const SkinCosmetic *const item{ skin_catalog->cosmetic(slot.def) };

		if (slot.def && (!item || !(item->classes & (1u << cls))))
		{
			slot = {};
			continue;
		}

		for (size_t prev{}; prev < n && item; prev++)
		{
			const SkinCosmetic *const other{ skin_catalog->cosmetic(override.slots[prev].def) };

			if (other && !skin_catalog->conflict(*item, *other).empty()) {
				slot = {};
				break;
			}
		}
	}

	m_player[cls] = override;

	nlohmann::ordered_json j = nlohmann::ordered_json::object();

	for (const auto &[name, id] : player_classes)
	{
		const ClassOverride &o{ m_player[id] };

		if (o == ClassOverride{}) {
			continue;
		}

		nlohmann::ordered_json slots = nlohmann::ordered_json::array();

		for (const CosmeticSlot &slot : o.slots) {
			slots.push_back({ { "def", slot.def }, { "paint", slot.paint }, { "style", slot.style }, { "unusual", slot.unusual } });
		}

		j[name] = { { "enabled", o.enabled }, { "slots", slots } };
	}

	cfg::skins_player = j.empty() ? "" : j.dump();
	m_player_parsed_from = cfg::skins_player;
}

//wearables are econ entities like weapons, the item netvars sit at the same offsets
static C_TFWeaponBase *econ(C_BaseEntity *const ent)
{
	return static_cast<C_TFWeaponBase *>(ent);
}

//C_BasePlayer::m_hMyWearables, a CUtlVector of handles
struct HandleVector final
{
	unsigned long *memory{};
	int allocated{};
	int grow_size{};
	int size{};
	unsigned long *elements{};
};

//the player's m_hMyWearables, nullptr when the offset couldn't be found
static const HandleVector *myWearables(C_TFPlayer *const local)
{
	//NETVAR(m_hMyWearables) resolves to the wrong place for a utl vector prop, so the offset comes from the recv table setup
	static const int offset{ s::C_BasePlayer_MyWearablesOffset.get() ? *reinterpret_cast<const int *>(s::C_BasePlayer_MyWearablesOffset.get()) : 0 };

	if (!local || offset <= 0) {
		return nullptr;
	}

	return reinterpret_cast<const HandleVector *>(reinterpret_cast<uintptr_t>(local) + offset);
}

//your cosmetic wearables (not shields, boots and other weapon slot ones), by entity index so the order holds across updates
static std::vector<C_BaseEntity *> realCosmetics(C_TFPlayer *const local)
{
	std::vector<C_BaseEntity *> out{};

	const HandleVector *const wearables{ myWearables(local) };

	if (!wearables) {
		return out;
	}

	for (int n{}; n < wearables->size && wearables->memory; n++)
	{
		IClientEntity *const client_ent{ i::ent_list->GetClientEntityFromHandle(CBaseHandle{ wearables->memory[n] }) };

		if (!client_ent) {
			continue;
		}

		C_BaseEntity *const ent{ client_ent->cast<C_BaseEntity>() };

		if (ent->m_hOwnerEntity().Get() == local && skin_catalog->cosmetic(econ(ent)->m_iItemDefinitionIndex())) {
			out.push_back(ent);
		}
	}

	std::sort(out.begin(), out.end(), [](C_BaseEntity *const a, C_BaseEntity *const b) { return a->entindex() < b->entindex(); });

	return out;
}

//paint, style and effect of one slot, on a real cosmetic or one we spawned
static void setSlotAttributes(CAttributeList *const list, const CosmeticSlot &slot, const int team)
{
	const std::vector<SkinPaint> &paints{ SkinCatalog::paints() };

	if (slot.paint > 0 && slot.paint <= static_cast<int>(paints.size()))
	{
		const SkinPaint &paint{ paints[slot.paint - 1] };

		setAttribute(list, attributes::set_item_tint_rgb, static_cast<float>(paint.red));
		setAttribute(list, attributes::set_item_tint_rgb_2, static_cast<float>(paint.blu ? paint.blu : paint.red));
	}

	//the real item's style index means nothing on another item
	setAttribute(list, attributes::item_style_override, static_cast<float>(std::max(slot.style, 0)));

	//0 also clears the real item's effect
	setAttribute(list, attributes::set_attached_particle, static_cast<float>(slot.unusual ? SkinCatalog::unusualForTeam(slot.unusual, team) : 0));
}

//a cosmetic the class can wear that hides nothing and takes no region, for real wearables the override has no slot for
static int blankCosmetic(const int cls)
{
	for (const SkinCosmetic &cosmetic : skin_catalog->cosmetics())
	{
		if ((cosmetic.classes & (1u << cls)) && !cosmetic.hides_bodygroups && !cosmetic.own && cosmetic.regions.empty()) {
			return cosmetic.def;
		}
	}

	return 0;
}

void SkinChanger::applyPlayer(C_TFPlayer *const local)
{
	m_hidden_wearables.clear();
	m_extra_slots.clear();

	parsePlayer();

	const int cls{ local->m_iClass() };

	if (!validClass(cls) || !m_player[cls].enabled) {
		return;
	}

	const ClassOverride &o{ m_player[cls] };
	const int team{ local->m_iTeamNum() };

	std::vector<const CosmeticSlot *> active{};

	for (const CosmeticSlot &slot : o.slots)
	{
		if (slot.def && skin_catalog->cosmetic(slot.def)) {
			active.push_back(&slot);
		}
	}

	const std::vector<C_BaseEntity *> wearables{ realCosmetics(local) };

	for (size_t n{}; n < wearables.size(); n++)
	{
		C_TFWeaponBase *const wearable{ econ(wearables[n]) };
		CAttributeList *const list{ wearable->m_AttributeList() };

		//nothing left to show on this one, it gets a harmless def (so its bodygroups and regions go) and stays out of sight
		if (n >= active.size())
		{
			if (const int blank{ active.empty() ? blankCosmetic(cls) : active.front()->def }) {
				wearable->m_iItemDefinitionIndex() = blank;
			}

			if (list) {
				setAttribute(list, attributes::set_attached_particle, 0.0f);
			}

			m_hidden_wearables.push_back(handleOf(wearables[n]));
			continue;
		}

		const CosmeticSlot &slot{ *active[n] };

		//skin and bodygroups follow the def by themselves, the model doesn't: the server sent the real item's,
		//and the game only works the model out from the def (ValidateModelIndex) on its own updates
		const bool swapped{ wearable->m_iItemDefinitionIndex() != slot.def };

		wearable->m_iItemDefinitionIndex() = slot.def;

		if (list) {
			setSlotAttributes(list, slot, team);
		}

		//C_EconEntity::ValidateModelIndex, after the attributes so the picked style's model counts
		if (swapped) {
			mem::callVirtual<236, void>(wearables[n]);
		}
	}

	//more slots than real cosmetics, run() spawns the rest
	for (size_t n{ wearables.size() }; n < active.size(); n++) {
		m_extra_slots.push_back(*active[n]);
	}
}

bool SkinChanger::onLevelInit()
{
	m_full_update_time = 0.0f;

	return true;
}

void SkinChanger::apply()
{
	if (!cfg::skins_active)
	{
		m_hidden_wearables.clear();
		m_extra_slots.clear();
		return;
	}

	C_TFPlayer *const local{ ec->getLocal() };

	if (!local || !skin_catalog->ensure()) {
		return;
	}

	parse();
	applyPlayer(local);

	for (int n{}; n < MAX_WEAPONS && !m_overrides.empty(); n++)
	{
		C_TFWeaponBase *const weapon{ local->GetWeapon(n) };

		if (!weapon) {
			continue;
		}

		//a swapped def is still in the same family, so this finds it again every frame
		const int def{ weapon->m_iItemDefinitionIndex() };
		const SkinFamily *const family{ skin_catalog->familyOf(def) };

		if (!family) {
			continue;
		}

		const auto it{ m_overrides.find(family->base) };

		if (it == m_overrides.end() || !it->second.enabled) {
			continue;
		}

		const WeaponOverride &o{ it->second };

		//every member has the same stats, only the looks differ
		const int target{ resolveVariant(*family, o, def) };

		if (target != def) {
			weapon->m_iItemDefinitionIndex() = target;
		}

		const SkinItem *const item{ skin_catalog->item(target) };
		CAttributeList *const list{ weapon->m_AttributeList() };

		if (!item || !list) {
			continue;
		}

		const bool has_kit{ o.paintkit && std::find(item->paintkits.begin(), item->paintkits.end(), o.paintkit) != item->paintkits.end() };

		if (has_kit)
		{
			//integer attributes keep their bits in the float
			setAttribute(list, attributes::paintkit_proto_def_index, std::bit_cast<float>(o.paintkit));
			setAttribute(list, attributes::set_item_texture_wear, wearValue(o.wear));

			if (o.seed) {
				setAttribute(list, attributes::custom_paintkit_seed_lo, std::bit_cast<float>(o.seed));
				setAttribute(list, attributes::custom_paintkit_seed_hi, 0.0f);
			}
		}

		//the gold look is style 1 of the australium-able items, a picked style wins over it
		if (o.australium && item->australium)
		{
			setAttribute(list, attributes::is_australium_item, 1.0f);

			if (o.style < 0) {
				setAttribute(list, attributes::item_style_override, 1.0f);
			}
		}

		if (o.style >= 0) {
			setAttribute(list, attributes::item_style_override, static_cast<float>(o.style));
		}

		if (o.festivized && item->festivizable) {
			setAttribute(list, attributes::is_festivized, 1.0f);
		}

		if (o.stattrak && item->stattrak)
		{
			setAttribute(list, attributes::kill_eater, std::bit_cast<float>(0));
			setAttribute(list, attributes::kill_eater_score_type, 0.0f);
		}

		//sheens need specialized, killstreakers professional
		if (o.ks_tier > 0)
		{
			setAttribute(list, attributes::killstreak_tier, static_cast<float>(std::clamp(o.ks_tier, 1, 3)));

			if (o.sheen > 0 && o.ks_tier >= 2) {
				setAttribute(list, attributes::killstreak_idleeffect, static_cast<float>(std::clamp(o.sheen, 1, 7)));
			}

			if (o.killstreaker > 0 && o.ks_tier >= 3) {
				setAttribute(list, attributes::killstreak_effect, static_cast<float>(2001 + std::clamp(o.killstreaker, 1, 7)));
			}
		}

		if (o.unusual) {
			setAttribute(list, attributes::set_attached_particle, unusualEffect(o.unusual));
		}

		//the game only draws spells while the halloween/full moon holiday is on
		if (o.spell_pumpkin && canPumpkin(*item)) {
			setAttribute(list, attributes::halloween_pumpkin_explosions, 1.0f);
		}

		if (o.spell_green_flames && canGreenFlames(*item)) {
			setAttribute(list, attributes::halloween_green_flames, 1.0f);
		}
	}
}

bool canPumpkin(const SkinItem &item)
{
	for (const char *const cls : { "rocketlauncher", "grenadelauncher", "pipebomblauncher", "cannon", "particle_cannon" })
	{
		if (item.item_class.find(cls) != std::string::npos) {
			return true;
		}
	}

	return false;
}

bool canGreenFlames(const SkinItem &item)
{
	return item.item_class.find("flamethrower") != std::string::npos;
}

//botkiller heads: what the server makes for a real botkiller, spawned client side. a tf_wearable bone merged onto the player and a
//tf_wearable_vm on the viewmodel arms (the head's skeleton is the hand and weapon bones), both tied to their weapon.
//the wearables' own ShouldDraw then shows them only while that weapon is out, the world one in third person and the vm one in first

constexpr int RENDER_GROUP_OPAQUE_ENTITY{ 7 };
constexpr int RENDER_GROUP_VIEW_MODEL_OPAQUE{ 10 };
constexpr int EFFECT_NODRAW{ 0x20 };
constexpr int EFFECT_BONEMERGE_FASTCULL{ 0x80 };
constexpr unsigned long INVALID_HANDLE{ 0xFFFFFFFF };

//C_BaseEntity::m_bReadyToDraw, C_BaseAnimating::DrawModel draws nothing without it.
//the interpolation pass sets it for networked entities, a client made one never goes through that
constexpr uintptr_t READY_TO_DRAW_OFFSET{ 0xC0 };
constexpr uintptr_t STUDIO_HDR_OFFSET{ 0xBF0 }; //C_BaseAnimating::m_pStudioHdr, null until the model has loaded

static bool &readyToDraw(C_BaseEntity *const ent)
{
	return *reinterpret_cast<bool *>(reinterpret_cast<uintptr_t>(ent) + READY_TO_DRAW_OFFSET);
}

static C_BaseEntity *fromHandle(const unsigned long handle)
{
	if (!handle || handle == INVALID_HANDLE) {
		return nullptr;
	}

	IClientEntity *const ent{ i::ent_list->GetClientEntityFromHandle(CBaseHandle{ handle }) };

	return ent ? ent->cast<C_BaseEntity>() : nullptr;
}

static unsigned long handleOf(C_BaseEntity *const ent)
{
	//IHandleEntity::GetRefEHandle
	return ent ? *mem::callVirtual<2, const unsigned long *>(ent) : INVALID_HANDLE;
}

//botkiller heads aren't precached unless someone in the server has that botkiller on
static bool ensureModel(const std::string &path)
{
	if (i::model_info->GetModelIndex(path.c_str()) != -1) {
		return true;
	}

	i::model_info->RegisterDynamicModel(path.c_str(), true);

	return i::model_info->GetModelIndex(path.c_str()) != -1;
}

static void setDrawn(C_BaseEntity *const ent, const bool drawn)
{
	if (!ent || ((ent->m_fEffects() & EFFECT_NODRAW) == 0) == drawn) {
		return;
	}

	ent->m_fEffects() = drawn ? ent->m_fEffects() & ~EFFECT_NODRAW : ent->m_fEffects() | EFFECT_NODRAW;

	//UpdateVisibility, what AddEffects/RemoveEffects call after touching EF_NODRAW
	mem::callVirtual<91, void>(ent);
}

//once per message, the spawn gets retried every frame
static void logOnce(const std::string &msg)
{
	static std::unordered_set<std::string> logged{};

	if (logged.insert(msg).second) {
		i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 }, "[skins] %s\n", msg.c_str());
	}
}

static unsigned long spawnHead(const std::string &path, C_TFPlayer *const local, C_BaseEntity *const parent, C_TFWeaponBase *const weapon, const bool view,
	const int def, const int skin)
{
	static const uintptr_t associated{ netvars->getOffset(HASH_CT("DT_TFWearable"), HASH_CT("m_hWeaponAssociatedWith"), "DT_TFWearable", "m_hWeaponAssociatedWith") };
	static const uintptr_t initialized{ netvars->getOffset(HASH_CT("DT_ScriptCreatedItem"), HASH_CT("m_bInitialized"), "DT_ScriptCreatedItem", "m_bInitialized") };

	const char *const which{ view ? "viewmodel" : "world" };

	if (!parent || path.empty()) {
		return INVALID_HANDLE;
	}

	void *const class_map{ s::GetClassMap.call<void *>() };

	if (!class_map || !associated || !initialized || !s::C_BaseEntity_FollowEntity.get())
	{
		logOnce(std::format("botkiller head: missing class map {} / m_hWeaponAssociatedWith {:#x} / m_bInitialized {:#x} / FollowEntity {}",
			class_map, associated, initialized, reinterpret_cast<void *>(s::C_BaseEntity_FollowEntity.get())));
		return INVALID_HANDLE;
	}

	if (!ensureModel(path))
	{
		logOnce(std::format("botkiller head {}: couldn't load {}", which, path));
		return INVALID_HANDLE;
	}

	//IClassMap::CreateEntity
	C_BaseEntity *const ent{ mem::callVirtual<3, C_BaseEntity *, const char *>(class_map, view ? "tf_wearable_vm" : "tf_wearable") };

	if (!ent)
	{
		logOnce(std::format("botkiller head {}: the game wouldn't create the wearable", which));
		return INVALID_HANDLE;
	}

	//InitializeAsClientEntity with the head's model. the item set below would pick the botkiller weapon's model instead,
	//but that only happens in ValidateModelIndex, which nothing calls on a client made entity
	if (!mem::callVirtual<82, bool, const char *, int>(ent, path.c_str(), view ? RENDER_GROUP_VIEW_MODEL_OPAQUE : RENDER_GROUP_OPAQUE_ENTITY))
	{
		logOnce(std::format("botkiller head {}: the game wouldn't take {}", which, path));

		//what the game does when this fails, it isn't in the entity list yet
		static_cast<IClientNetworkable *>(ent)->Release();
		return INVALID_HANDLE;
	}

	*reinterpret_cast<unsigned long *>(&ent->m_hOwnerEntity()) = handleOf(local);
	*reinterpret_cast<unsigned long *>(reinterpret_cast<uintptr_t>(ent) + associated) = handleOf(weapon);
	ent->m_iTeamNum() = local->m_iTeamNum();

	//the botkiller's own item, like the server's head carries. the wearable takes its skin (the tier colour) from the item's
	//visuals_red/blu, without one it's the plain team skin, silver
	econ(ent)->m_iItemDefinitionIndex() = def;
	*reinterpret_cast<bool *>(reinterpret_cast<uintptr_t>(ent) + initialized) = true;

	s::C_BaseEntity_FollowEntity.call<void>(ent, parent, true);
	ent->m_fEffects() |= EFFECT_BONEMERGE_FASTCULL;

	ent->cast<C_BaseAnimating>()->m_nSkin() = skin;
	readyToDraw(ent) = true;

	logOnce(std::format("botkiller head {}: spawned {}", which, path));

	return handleOf(ent);
}

static void removeEntity(const unsigned long handle)
{
	if (C_BaseEntity *const ent{ fromHandle(handle) }) {
		s::C_BaseEntity_Remove.call<void>(ent);
	}
}

void SkinChanger::removeHeads()
{
	for (const BotkillerHead &head : m_heads)
	{
		removeEntity(head.world);
		removeEntity(head.view);
	}

	m_heads.clear();
}

//cosmetics past your real ones: the game's own tf_wearable, made client side, so its econ code does the model, skin, style, paint and effect
constexpr int LIFE_STATE_ALIVE{ 0 };

static unsigned long spawnCosmetic(C_TFPlayer *const local, const CosmeticSlot &slot, const int team)
{
	static const uintptr_t initialized{ netvars->getOffset(HASH_CT("DT_ScriptCreatedItem"), HASH_CT("m_bInitialized"), "DT_ScriptCreatedItem", "m_bInitialized") };

	void *const class_map{ s::GetClassMap.call<void *>() };

	if (!class_map || !initialized || !s::C_BaseEntity_FollowEntity.get())
	{
		logOnce(std::format("cosmetic: missing class map {} / m_bInitialized {:#x} / FollowEntity {}", class_map, initialized, reinterpret_cast<void *>(s::C_BaseEntity_FollowEntity.get())));
		return INVALID_HANDLE;
	}

	//IClassMap::CreateEntity, the same factory the game uses for the map name
	C_BaseEntity *const ent{ mem::callVirtual<3, C_BaseEntity *, const char *>(class_map, "tf_wearable") };

	if (!ent)
	{
		logOnce("cosmetic: the game wouldn't create a tf_wearable");
		return INVALID_HANDLE;
	}

	//InitializeAsClientEntity, no model yet (that can't fail), the item picks one below
	mem::callVirtual<82, bool, const char *, int>(ent, nullptr, RENDER_GROUP_OPAQUE_ENTITY);

	//the model lookup wants the owner's class and team
	*reinterpret_cast<unsigned long *>(&ent->m_hOwnerEntity()) = handleOf(local);
	ent->m_iTeamNum() = team;

	C_TFWeaponBase *const item{ econ(ent) };

	item->m_iItemDefinitionIndex() = slot.def;
	*reinterpret_cast<bool *>(reinterpret_cast<uintptr_t>(ent) + initialized) = true;

	if (CAttributeList *const list{ item->m_AttributeList() }) {
		setSlotAttributes(list, slot, team);
	}

	//bone merged onto the player like a real wearable
	s::C_BaseEntity_FollowEntity.call<void>(ent, local, true);
	ent->m_fEffects() |= EFFECT_BONEMERGE_FASTCULL;

	//C_EconEntity::ValidateModelIndex, registers the item's (dynamic) model and sets it, OnNewModel waits for it to load
	mem::callVirtual<236, void>(ent);

	//what a server made wearable gets on creation: attributes, unusual particles, the owner's bodygroups refreshed
	static_cast<IClientNetworkable *>(ent)->OnDataChanged(DATA_UPDATE_CREATED);

	readyToDraw(ent) = true;

	//TEMP spawned cosmetic debugging
	i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 }, "[skins] cosmetic def %d spawned: %p handle 0x%lX model %p index %d effects 0x%X\n",
		slot.def, ent, handleOf(ent), ent->GetModel(), ent->m_nModelIndex(), ent->m_fEffects());

	return handleOf(ent);
}

//mstudiobodyparts_t
struct StudioBodyPart final
{
	int name_index{};
	int num_models{};
	int base{};
	int model_index{};
};

//::SetBodygroup by name, on a body value instead of an entity
static void setBodygroup(const studiohdr_t *const hdr, int &body, const std::string &name, const int value)
{
	for (int n{}; n < hdr->numbodyparts; n++)
	{
		const StudioBodyPart *const part{ reinterpret_cast<const StudioBodyPart *>(reinterpret_cast<uintptr_t>(hdr) + hdr->bodypartindex) + n };
		const char *const part_name{ reinterpret_cast<const char *>(part) + part->name_index };

		if (_stricmp(part_name, name.c_str())) {
			continue;
		}

		if (value < 0 || value >= part->num_models || part->base <= 0) {
			return;
		}

		const int current{ (body / part->base) % part->num_models };

		body += (value - current) * part->base;
		return;
	}
}

//the player hides a real wearable's bodygroups itself (helmets under hats, hat hair, headphones) but only walks m_hMyWearables,
//so the spawned ones' go on top of whatever the game last set. reapplied every frame, the game resets m_nBody whenever it redoes its own
void SkinChanger::updateBodygroups(C_TFPlayer *const local)
{
	int &body{ local->m_nBody() };

	//a new player object (full update, new map), what we wrote was for the old one
	if (m_body_player != reinterpret_cast<uintptr_t>(local))
	{
		m_body_player = reinterpret_cast<uintptr_t>(local);
		m_body_written = -1;
	}

	//anything else than our own value is the game's
	if (body != m_body_written) {
		m_body_base = body;
	}

	std::vector<std::string> groups{};

	for (const SpawnedCosmetic &spawned : m_spawned)
	{
		if (const SkinCosmetic *const cosmetic{ skin_catalog->cosmetic(spawned.slot.def) }) {
			groups.insert(groups.end(), cosmetic->bodygroups.begin(), cosmetic->bodygroups.end());
		}
	}

	const model_t *const model{ local->GetModel() };
	const studiohdr_t *const hdr{ model ? i::model_info->GetStudioModel(model) : nullptr };

	if (groups.empty() || !hdr)
	{
		//put the game's back
		if (m_body_written != -1 && body == m_body_written) {
			body = m_body_base;
		}

		m_body_written = -1;
		return;
	}

	int next{ m_body_base };

	for (const std::string &group : groups) {
		setBodygroup(hdr, next, group, 1);
	}

	body = next;
	m_body_written = next;
}

void SkinChanger::removeSpawned()
{
	for (const SpawnedCosmetic &spawned : m_spawned) {
		removeEntity(spawned.handle);
	}

	m_spawned.clear();

	//the bodygroups only they hid come back
	C_TFPlayer *const local{ i::engine->IsInGame() ? ec->getLocal() : nullptr };

	if (local && m_body_player == reinterpret_cast<uintptr_t>(local) && m_body_written != -1 && local->m_nBody() == m_body_written) {
		local->m_nBody() = m_body_base;
	}

	m_body_written = -1;
}

void SkinChanger::updateSpawned()
{
	C_TFPlayer *const local{ cfg::skins_active && i::engine->IsInGame() ? ec->getLocal() : nullptr };

	if (!local || !skin_catalog->ensure())
	{
		removeSpawned();
		return;
	}

	const int team{ local->m_iTeamNum() };

	//nothing to wear while dead, they come back on respawn
	const size_t wanted{ local->m_lifeState() == LIFE_STATE_ALIVE ? m_extra_slots.size() : 0 };

	//slot by slot, everything from the first one that changed gets respawned.
	//a full update recreates the player under the same handle, so the parent is checked by pointer
	size_t keep{};

	while (keep < m_spawned.size() && keep < wanted)
	{
		const SpawnedCosmetic &spawned{ m_spawned[keep] };
		C_BaseEntity *const ent{ fromHandle(spawned.handle) };

		if (!ent || ent->GetMoveParent().Get() != local || spawned.team != team || !(spawned.slot == m_extra_slots[keep])) {
			break;
		}

		keep++;
	}

	for (size_t n{ keep }; n < m_spawned.size(); n++) {
		removeEntity(m_spawned[n].handle);
	}

	m_spawned.resize(keep);

	for (size_t n{ keep }; n < wanted; n++)
	{
		const unsigned long handle{ spawnCosmetic(local, m_extra_slots[n], team) };

		//tried again next frame
		if (handle == INVALID_HANDLE) {
			break;
		}

		m_spawned.push_back({ handle, m_extra_slots[n], team });
	}

	updateBodygroups(local);

	//TEMP debugging: whether anything cleared it since last frame
	static bool was_cleared{};

	//kept set every frame in case anything clears it
	for (const SpawnedCosmetic &spawned : m_spawned)
	{
		if (C_BaseEntity *const ent{ fromHandle(spawned.handle) })
		{
			was_cleared |= !readyToDraw(ent);
			readyToDraw(ent) = true;

			//UpdateVisibility: in or out of the render lists by ShouldDraw. the game only calls it on changes it knows about
			//(network updates, model loads, the player's third person switch walking m_hMyWearables), none reach ours
			mem::callVirtual<91, void>(ent);
		}
	}

	//TEMP spawned cosmetic debugging, every 3 seconds while the override wants any
	static float next_dump{};

	if (!m_extra_slots.empty() && i::global_vars->realtime >= next_dump)
	{
		next_dump = i::global_vars->realtime + 3.0f;

		i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 }, "[skins] cosmetics: %d real, %d extra slots, %d spawned, alive %d, first person %d, ready flag got cleared %d\n",
			static_cast<int>(realCosmetics(local).size()), static_cast<int>(m_extra_slots.size()), static_cast<int>(m_spawned.size()), local->m_lifeState() == LIFE_STATE_ALIVE,
			local->InFirstPersonView(), was_cleared);

		was_cleared = false;

		//TEMP: why a real cosmetic isn't counted, everything in m_hMyWearables
		const HandleVector *const wearables{ myWearables(local) };
		const HandleVector empty{};
		const HandleVector &list{ wearables ? *wearables : empty };

		i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 }, "[skins]   m_hMyWearables: found %d size %d memory %p, body %d (game's %d)\n",
			wearables != nullptr, list.size, list.memory, local->m_nBody(), m_body_base);

		for (int n{}; n < list.size && list.memory && n < 16; n++)
		{
			C_BaseEntity *const ent{ fromHandle(list.memory[n]) };

			if (!ent)
			{
				i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 }, "[skins]     wearable 0x%lX: not here\n", list.memory[n]);
				continue;
			}

			const int def{ econ(ent)->m_iItemDefinitionIndex() };

			i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 }, "[skins]     wearable 0x%lX: class %d def %d owner ok %d in catalog %d dormant %d\n",
				list.memory[n], static_cast<int>(ent->GetClassId()), def, ent->m_hOwnerEntity().Get() == local, skin_catalog->cosmetic(def) != nullptr, ent->IsDormant());
		}

		for (const SpawnedCosmetic &spawned : m_spawned)
		{
			C_BaseEntity *const ent{ fromHandle(spawned.handle) };

			if (!ent)
			{
				i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 }, "[skins]   def %d: gone\n", spawned.slot.def);
				continue;
			}

			const short index{ static_cast<short>(ent->m_nModelIndex()) };
			const Vector origin{ ent->GetAbsOrigin() };
			const Vector player{ local->GetAbsOrigin() };

			i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 },
				"[skins]   def %d: %p model %p index %d loading %d studio %p effects 0x%X render mode %d parent ok %d should draw %d skin %d at %.0f %.0f %.0f (you %.0f %.0f %.0f)\n",
				spawned.slot.def, ent, ent->GetModel(), index, index < -1 && i::model_info->IsDynamicModelLoading(index),
				*reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(ent) + STUDIO_HDR_OFFSET), ent->m_fEffects(), ent->m_nRenderMode(),
				ent->GetMoveParent().Get() == local, ent->ShouldDraw(), ent->cast<C_BaseAnimating>()->m_nSkin(),
				origin.x, origin.y, origin.z, player.x, player.y, player.z);
		}
	}
}

constexpr bool BOTKILLER_HEADS{ true };

//the botkiller head a weapon should carry right now, nullptr for none
static const SkinItem *wantedHead(C_TFWeaponBase *const weapon, const std::unordered_map<int, WeaponOverride> &overrides)
{
	if (!BOTKILLER_HEADS) {
		return nullptr;
	}

	const SkinFamily *const family{ skin_catalog->familyOf(weapon->m_iItemDefinitionIndex()) };

	if (!family) {
		return nullptr;
	}

	const auto it{ overrides.find(family->base) };

	if (it == overrides.end() || !it->second.enabled || !it->second.botkiller) {
		return nullptr;
	}

	const SkinItem *const head{ skin_catalog->item(it->second.botkiller) };

	return head && !head->extra_wearable.empty() ? head : nullptr;
}

//a real botkiller's head is a wearable the server spawned, ours replaces it
static void hideServerHead(C_TFWeaponBase *const weapon)
{
	static const uintptr_t world{ netvars->getOffset(HASH_CT("DT_TFWeaponBase"), HASH_CT("m_hExtraWearable"), "DT_TFWeaponBase", "m_hExtraWearable") };
	static const uintptr_t view{ netvars->getOffset(HASH_CT("DT_TFWeaponBase"), HASH_CT("m_hExtraWearableViewModel"), "DT_TFWeaponBase", "m_hExtraWearableViewModel") };

	for (const uintptr_t offset : { world, view })
	{
		if (!offset) {
			continue;
		}

		//the weapon puts these back on deploy, so this runs every frame
		setDrawn(fromHandle(*reinterpret_cast<const unsigned long *>(reinterpret_cast<uintptr_t>(weapon) + offset)), false);
	}
}

//m_hViewModel is an old style recv array the netvar dump skips, so it's found by owner instead
static C_BaseEntity *localViewModel(C_TFPlayer *const local)
{
	static unsigned long cached{ INVALID_HANDLE };

	if (C_BaseEntity *const ent{ fromHandle(cached) }) {
		return ent;
	}

	for (int n{ 1 }; n <= i::ent_list->GetHighestEntityIndex(); n++)
	{
		IClientEntity *const client_ent{ i::ent_list->GetClientEntity(n) };

		if (!client_ent || client_ent->GetClassId() != class_ids::CTFViewModel) {
			continue;
		}

		C_BaseViewModel *const vm{ client_ent->cast<C_BaseViewModel>() };

		if (vm->m_hOwner().Get() == local && vm->m_nViewModelIndex() == 0)
		{
			cached = handleOf(vm);
			return vm;
		}
	}

	return nullptr;
}

void SkinChanger::updateHeads()
{
	C_TFPlayer *const local{ cfg::skins_active && i::engine->IsInGame() ? ec->getLocal() : nullptr };

	if (!local || !skin_catalog->ensure())
	{
		removeHeads();
		return;
	}

	parse();

	C_BaseEntity *const active{ local->m_hActiveWeapon().Get() };
	const bool first_person{ local->InFirstPersonView() };
	const int team{ local->m_iTeamNum() };

	//what should exist: weapon handle -> head
	std::vector<std::pair<C_TFWeaponBase *, const SkinItem *>> wanted{};

	for (int n{}; n < MAX_WEAPONS; n++)
	{
		C_TFWeaponBase *const weapon{ local->GetWeapon(n) };

		if (!weapon) {
			continue;
		}

		if (const SkinItem *const head{ wantedHead(weapon, m_overrides) }) {
			wanted.emplace_back(weapon, head);
		}
	}

	//drop heads whose weapon went away (full updates recreate them) or whose setting changed
	for (auto it{ m_heads.begin() }; it != m_heads.end();)
	{
		C_BaseEntity *const weapon{ fromHandle(it->weapon) };

		const auto match{ std::find_if(wanted.begin(), wanted.end(), [&](const auto &w) { return w.first == weapon && w.second->def == it->def; }) };

		if (!weapon || match == wanted.end() || it->team != team)
		{
			removeEntity(it->world);
			removeEntity(it->view);
			it = m_heads.erase(it);
			continue;
		}

		++it;
	}

	for (const auto &[weapon, item] : wanted)
	{
		const unsigned long weapon_handle{ handleOf(weapon) };

		auto it{ std::find_if(m_heads.begin(), m_heads.end(), [&](const BotkillerHead &h) { return h.weapon == weapon_handle; }) };

		//the tier colours are team skins, silver and diamond only have the model's own
		const int skin{ team == TF_TEAM_BLU ? (item->skin_blu >= 0 ? item->skin_blu : 1) : (item->skin_red >= 0 ? item->skin_red : 0) };

		if (it == m_heads.end())
		{
			BotkillerHead head{};

			head.weapon = weapon_handle;
			head.def = item->def;
			head.team = team;
			head.world = INVALID_HANDLE;
			head.view = INVALID_HANDLE;

			m_heads.push_back(head);
			it = m_heads.end() - 1;
		}

		//a full update recreates the player, viewmodel and weapons under the same handles, which leaves our heads parentless where they were.
		//so the parent is checked by pointer, and a head on the wrong one is respawned
		C_BaseEntity *const view_parent{ localViewModel(local) };
		C_BaseEntity *const world{ fromHandle(it->world) };
		C_BaseEntity *const view{ fromHandle(it->view) };

		//the real head is a wearable on the player, merged onto the player's hand and weapon bones, ours follows the same skeleton.
		//a failed spawn gets another go next frame
		if (!world || world->GetMoveParent().Get() != local || it->weapon_ent != reinterpret_cast<uintptr_t>(weapon))
		{
			removeEntity(it->world);
			it->world = spawnHead(item->extra_wearable, local, local, weapon, false, item->def, skin);
		}

		//first person it's on the viewmodel arms, like the real head's viewmodel wearable
		if (!view || !view_parent || view->GetMoveParent().Get() != view_parent || it->weapon_ent != reinterpret_cast<uintptr_t>(weapon))
		{
			removeEntity(it->view);

			const std::string &vm_path{ item->extra_wearable_vm.empty() ? item->extra_wearable : item->extra_wearable_vm };

			it->view = spawnHead(vm_path, local, view_parent, weapon, true, item->def, skin);
		}

		it->weapon_ent = reinterpret_cast<uintptr_t>(weapon);

		hideServerHead(weapon);

		//same as the spawned cosmetics: DrawModel skips it without the flag, and nothing else puts it in the render lists
		//when its ShouldDraw answer changes (weapon switch, first/third person)
		for (const unsigned long handle : { it->world, it->view })
		{
			if (C_BaseEntity *const ent{ fromHandle(handle) })
			{
				readyToDraw(ent) = true;
				mem::callVirtual<91, void>(ent);
			}
		}
	}

	//TEMP botkiller debugging, every 3 seconds while there's a head
	static float next_dump{};

	if (!m_heads.empty() && i::global_vars->realtime >= next_dump)
	{
		next_dump = i::global_vars->realtime + 3.0f;

		for (const BotkillerHead &head : m_heads)
		{
			for (const unsigned long handle : { head.world, head.view })
			{
				C_BaseEntity *const ent{ fromHandle(handle) };

				if (!ent)
				{
					i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 }, "[skins] head def %d %s: none\n", head.def, handle == head.world ? "world" : "view");
					continue;
				}

				const Vector origin{ ent->GetAbsOrigin() };

				i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 },
					"[skins] head def %d %s: %p model %p studio %p effects 0x%X parent %p should draw %d at %.0f %.0f %.0f, first person %d, weapon out %d\n",
					head.def, handle == head.world ? "world" : "view", ent, ent->GetModel(), *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(ent) + STUDIO_HDR_OFFSET),
					ent->m_fEffects(), ent->GetMoveParent().Get(), ent->ShouldDraw(), origin.x, origin.y, origin.z, first_person,
					head.weapon_ent == reinterpret_cast<uintptr_t>(active));
			}
		}
	}
}

bool SkinChanger::onLevelShutdown()
{
	removeHeads();
	removeSpawned();

	return true;
}

bool SkinChanger::onUnload()
{
	removeHeads();
	removeSpawned();

	return true;
}

void SkinChanger::applyNow()
{
	if (i::engine->IsInGame()) {
		i::client_state->ForceFullUpdate();
	}

	m_full_update_time = 0.0f;
	m_applied = settingsKey();
}

void SkinChanger::run()
{
	updateHeads();
	updateSpawned();

	if (!i::engine->IsInGame()) {
		return;
	}

	if (!cfg::skins_active) {
		m_hidden_wearables.clear();
	}

	//ShouldDraw bails on a render mode of none, and the server only sends render mode when it changes, so this sticks
	for (const unsigned long handle : m_hidden_wearables)
	{
		if (C_BaseEntity *const ent{ fromHandle(handle) }) {
			ent->m_nRenderMode() = RENDER_MODE_NONE;
		}
	}

	std::string key{ settingsKey() };

	const float now{ i::global_vars->realtime };

	if (key != m_applied)
	{
		m_applied = std::move(key);

		//waits a moment so flicking through the lists doesn't rebuild every frame
		m_full_update_time = now + std::max(cfg::skins_apply_delay, 0.0f);
	}

	//weapons only build their looks when they're created, a full update recreates them.
	//turning something off goes through here too, the recreated weapons come back with the server's def and attributes
	if (m_full_update_time > 0.0f && now >= m_full_update_time)
	{
		m_full_update_time = 0.0f;
		i::client_state->ForceFullUpdate();

		//TEMP: checking whether the hitch after picking a head is full updates firing over and over
		i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 }, "[skins] full update at %.1f\n", now);
	}
}
