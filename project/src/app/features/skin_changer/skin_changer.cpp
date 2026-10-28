#include "skin_changer.hpp"

#include "../cfg.hpp"

#include <bit>
#include <unordered_set>

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

bool SkinChanger::onLevelInit()
{
	m_full_update_time = 0.0f;

	return true;
}

void SkinChanger::apply()
{
	if (!cfg::skins_active) {
		return;
	}

	C_TFPlayer *const local{ ec->getLocal() };

	if (!local || !skin_catalog->ensure()) {
		return;
	}

	parse();

	if (m_overrides.empty()) {
		return;
	}

	for (int n{}; n < MAX_WEAPONS; n++)
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

//botkiller heads: the game's own bone merged attachment model (what carries the stattrak module), spawned client side.
//the head's skeleton is the player's hand and weapon bones, so it's bone merged onto the player (and the viewmodel arms in first person) like the real wearable

constexpr size_t ATTACHMENT_MODEL_SIZE{ 0xC28 }; //what the game allocates for one
constexpr int RENDER_GROUP_OPAQUE_ENTITY{ 7 };
constexpr int RENDER_GROUP_VIEW_MODEL_OPAQUE{ 10 };
constexpr int EFFECT_NODRAW{ 0x20 };
constexpr unsigned long INVALID_HANDLE{ 0xFFFFFFFF };

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
static void headLog(const std::string &msg)
{
	static std::unordered_set<std::string> logged{};

	if (logged.insert(msg).second) {
		i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 }, "[skins] %s\n", msg.c_str());
	}
}

static unsigned long spawnHead(const std::string &path, C_BaseEntity *const parent, C_TFWeaponBase *const weapon, const bool view, const int skin)
{
	const char *const which{ view ? "viewmodel" : "world" };

	if (!parent || path.empty()) {
		return INVALID_HANDLE;
	}

	if (!ensureModel(path))
	{
		headLog(std::format("botkiller head {}: couldn't load {}", which, path));
		return INVALID_HANDLE;
	}

	void *const memory{ i::mem_alloc->Alloc(ATTACHMENT_MODEL_SIZE) };

	if (!memory) {
		return INVALID_HANDLE;
	}

	memset(memory, 0, ATTACHMENT_MODEL_SIZE);

	C_BaseEntity *const ent{ s::C_TFWeaponAttachmentModel_Ctor.call<C_BaseEntity *>(memory) };

	//InitializeAsClientEntity, only fails before the entity list knows about it
	if (!ent || !mem::callVirtual<82, bool, const char *, int>(ent, path.c_str(), view ? RENDER_GROUP_VIEW_MODEL_OPAQUE : RENDER_GROUP_OPAQUE_ENTITY))
	{
		headLog(std::format("botkiller head {}: the game wouldn't create it for {}", which, path));
		i::mem_alloc->Free(memory);
		return INVALID_HANDLE;
	}

	s::C_TFWeaponAttachmentModel_Init.call<void>(ent, parent, weapon, view);

	ent->cast<C_BaseAnimating>()->m_nSkin() = skin;

	//Init leaves it EF_NODRAW for its weapon to draw, ours stay in the renderable lists and the ShouldDraw hook decides
	setDrawn(ent, true);

	headLog(std::format("botkiller head {}: spawned {}", which, path));

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

//off until the heads render right, a saved head in a config would otherwise still hide a real botkiller's head
constexpr bool BOTKILLER_HEADS{ false };

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

//TEMP botkiller debugging: how often our heads got asked to draw since the last state dump
static int world_asks{};
static int view_asks{};

std::optional<bool> SkinChanger::headShouldDraw(const uintptr_t ent) const
{
	for (const BotkillerHead &head : m_heads)
	{
		if (head.world_ent && ent == head.world_ent) {
			world_asks++;
			return head.world_drawn;
		}

		if (head.view_ent && ent == head.view_ent) {
			view_asks++;
			return head.view_drawn;
		}
	}

	return std::nullopt;
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
			head.view_parent = INVALID_HANDLE;

			m_heads.push_back(head);
			it = m_heads.end() - 1;
		}

		//the real head is a wearable on the player, merged onto the player's hand and weapon bones, ours follows the same skeleton.
		//a model still loading fails the spawn, it gets another go next frame
		if (!fromHandle(it->world)) {
			it->world = spawnHead(item->extra_wearable, local, weapon, false, skin);
		}

		//first person it's the viewmodel arms, like the real head's viewmodel wearable
		const unsigned long view_parent{ handleOf(localViewModel(local)) };

		if (view_parent != it->view_parent || !fromHandle(it->view))
		{
			removeEntity(it->view);

			const std::string &vm_path{ item->extra_wearable_vm.empty() ? item->extra_wearable : item->extra_wearable_vm };

			it->view_parent = view_parent;
			it->view = spawnHead(vm_path, fromHandle(view_parent), weapon, true, skin);
		}

		hideServerHead(weapon);

		//only on the weapon that's out, and only in the view that's showing it
		const bool out{ weapon == active };

		it->world_ent = reinterpret_cast<uintptr_t>(fromHandle(it->world));
		it->view_ent = reinterpret_cast<uintptr_t>(fromHandle(it->view));
		it->world_drawn = out && !first_person;
		it->view_drawn = out && first_person;
	}

	//TEMP botkiller debugging, every 3 seconds while there's a head
	static float next_dump{};

	if (!m_heads.empty() && i::global_vars->realtime >= next_dump)
	{
		next_dump = i::global_vars->realtime + 3.0f;

		for (const BotkillerHead &head : m_heads)
		{
			C_BaseEntity *const world{ fromHandle(head.world) };
			C_BaseEntity *const view{ fromHandle(head.view) };

			i::cvar->ConsoleColorPrintf({ 255, 170, 0, 255 },
				"[skins] head def %d: world %p (effects 0x%X, drawn %d, asked %d) view %p (effects 0x%X, drawn %d, asked %d) view parent %p first person %d\n",
				head.def, world, world ? world->m_fEffects() : 0, head.world_drawn, world_asks,
				view, view ? view->m_fEffects() : 0, head.view_drawn, view_asks, fromHandle(head.view_parent), first_person);
		}

		world_asks = 0;
		view_asks = 0;
	}
}

bool SkinChanger::onLevelShutdown()
{
	removeHeads();

	return true;
}

bool SkinChanger::onUnload()
{
	removeHeads();

	return true;
}

void SkinChanger::applyNow()
{
	if (i::engine->IsInGame()) {
		i::client_state->ForceFullUpdate();
	}

	m_full_update_time = 0.0f;
	m_applied = cfg::skins_active ? cfg::skins_overrides : "off";
}

void SkinChanger::run()
{
	updateHeads();

	if (!i::engine->IsInGame()) {
		return;
	}

	//"off" and the empty overrides string mean the same: nothing to show
	std::string key{ cfg::skins_active ? cfg::skins_overrides : "" };

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
	}
}
