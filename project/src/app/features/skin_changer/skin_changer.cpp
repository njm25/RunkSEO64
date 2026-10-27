#include "skin_changer.hpp"

#include "../statistics/statistics.hpp"
#include "../cfg.hpp"

#include <bit>

struct SlotSettings final
{
	int kit{};
	int unusual{};
	int sheen{};
	bool australium{};
	bool festive{};
	int style{};
};

static SlotSettings slotSettings(const int slot)
{
	switch (slot)
	{
		case 0: return { cfg::skins_primary, cfg::skins_primary_unusual, cfg::skins_primary_sheen, cfg::skins_primary_australium, cfg::skins_primary_festive, cfg::skins_primary_style };
		case 1: return { cfg::skins_secondary, cfg::skins_secondary_unusual, cfg::skins_secondary_sheen, cfg::skins_secondary_australium, cfg::skins_secondary_festive, cfg::skins_secondary_style };
		case 2: return { cfg::skins_melee, cfg::skins_melee_unusual, cfg::skins_melee_sheen, cfg::skins_melee_australium, cfg::skins_melee_festive, cfg::skins_melee_style };
		default: return {};
	}
}

static std::string settingsKey()
{
	if (!cfg::skins_active) {
		return "off";
	}

	std::string key{ std::format("wear {}", cfg::skins_wear) };

	for (int slot{}; slot < 3; slot++)
	{
		const SlotSettings s{ slotSettings(slot) };

		key += std::format(" | {} {} {} {} {} {}", s.kit, s.unusual, s.sheen, s.australium, s.festive, s.style);
	}

	return key;
}

//the four effects weapons can have, in the order the menu lists them
static float unusualEffect(const int option)
{
	constexpr int effects[]{ 0, weapon_unusual_effects::weapon_unusual_hot, weapon_unusual_effects::weapon_unusual_isotope, weapon_unusual_effects::weapon_unusual_cool, weapon_unusual_effects::weapon_unusual_energyorb };

	return static_cast<float>(effects[std::clamp(option, 0, 4)]);
}

static float wearValue()
{
	//factory new to battle scarred
	return 0.2f + 0.2f * static_cast<float>(std::clamp(cfg::skins_wear, 0, 4));
}

static bool kitValid(C_TFWeaponBase *const weapon, const int kit)
{
	CUtlVector<int> *const kits{ tf_utils::getValidPaintKits(weapon) };

	if (!kits) {
		return false;
	}

	for (int n{}; n < kits->Count(); n++)
	{
		if ((*kits)[n] == kit) {
			return true;
		}
	}

	return false;
}

static C_TFWeaponBase *weaponInSlot(C_TFPlayer *const local, const int slot)
{
	for (int n{}; n < MAX_WEAPONS; n++)
	{
		C_TFWeaponBase *const weapon{ local->GetWeapon(n) };

		if (weapon && weapon->GetSlot() == slot) {
			return weapon;
		}
	}

	return nullptr;
}

//setting an attribute marks the item's attribute cache dirty, so only when it's actually different
static void setAttribute(CAttributeList *const list, const uint16_t index, const float value)
{
	for (int n{}; n < list->m_Attributes.Count(); n++)
	{
		const CEconItemAttribute &attr{ list->m_Attributes[n] };

		if (attr.m_iAttributeDefinitionIndex == index && attr.m_flValue == value) {
			return;
		}
	}

	list->SetAttribute(index, value);
}

SkinSlot skinSlot(const int slot)
{
	SkinSlot out{};

	C_TFPlayer *const local{ i::engine->IsInGame() ? ec->getLocal() : nullptr };

	if (!local) {
		return out;
	}

	C_TFWeaponBase *const weapon{ weaponInSlot(local, slot) };

	if (!weapon) {
		return out;
	}

	const int def_index{ weapon->m_iItemDefinitionIndex() };
	const char *const name{ weaponName(def_index) };

	out.weapon = name ? name : std::format("item {}", def_index);

	if (CUtlVector<int> *const kits{ tf_utils::getValidPaintKits(weapon) })
	{
		for (int n{}; n < kits->Count(); n++)
		{
			const int id{ (*kits)[n] };
			const std::string kit_name{ tf_utils::getPaintKitName(id) };

			out.kits.push_back(SkinOption{ id, kit_name.empty() ? std::format("paint kit {}", id) : kit_name });
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

	if (!local) {
		return;
	}

	for (int slot{}; slot < 3; slot++)
	{
		const SlotSettings s{ slotSettings(slot) };
		C_TFWeaponBase *const weapon{ weaponInSlot(local, slot) };

		if (!weapon) {
			continue;
		}

		CAttributeList *const list{ weapon->m_AttributeList() };

		if (!list) {
			continue;
		}

		//a kit picked for a different weapon than the one in the slot now is skipped, the rest still applies
		if (s.kit && kitValid(weapon, s.kit))
		{
			//integer attributes keep their bits in the float
			setAttribute(list, attributes::paintkit_proto_def_index, std::bit_cast<float>(s.kit));
			setAttribute(list, attributes::set_item_texture_wear, wearValue());
		}

		if (s.unusual) {
			setAttribute(list, attributes::set_attached_particle, unusualEffect(s.unusual));
		}

		//sheens come with the specialized killstreak tier
		if (s.sheen) {
			setAttribute(list, attributes::killstreak_tier, 2.0f);
			setAttribute(list, attributes::killstreak_idleeffect, static_cast<float>(s.sheen));
		}

		if (s.australium) {
			setAttribute(list, attributes::is_australium_item, 1.0f);
		}

		if (s.festive) {
			setAttribute(list, attributes::is_festivized, 1.0f);
		}

		if (s.style) {
			setAttribute(list, attributes::item_style_override, static_cast<float>(s.style - 1));
		}
	}
}

void SkinChanger::run()
{
	if (!i::engine->IsInGame()) {
		return;
	}

	//empty until the first check, nothing applied yet means nothing to rebuild
	std::string key{ settingsKey() };

	if (m_applied.empty()) {
		m_applied = cfg::skins_active ? "off" : key;
	}

	const float now{ i::global_vars->realtime };

	if (key != m_applied)
	{
		m_applied = std::move(key);

		//waits a moment so flicking through the list doesn't rebuild every frame
		m_full_update_time = now + 0.4f;
	}

	//weapons only build their paint when they're created, a full update recreates them.
	//turning it off goes through here too, the recreated weapons come back without the runtime attributes
	if (m_full_update_time > 0.0f && now >= m_full_update_time)
	{
		m_full_update_time = 0.0f;
		i::client_state->ForceFullUpdate();
	}
}
