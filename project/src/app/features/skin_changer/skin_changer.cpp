#include "skin_changer.hpp"

#include "../statistics/statistics.hpp"
#include "../cfg.hpp"

#include <bit>

static int kitForSlot(const int slot)
{
	switch (slot)
	{
		case 0: return cfg::skins_primary;
		case 1: return cfg::skins_secondary;
		case 2: return cfg::skins_melee;
		default: return 0;
	}
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
		const int kit{ kitForSlot(slot) };

		if (!kit) {
			continue;
		}

		C_TFWeaponBase *const weapon{ weaponInSlot(local, slot) };

		//a kit picked for a different weapon than the one in the slot now
		if (!weapon || !kitValid(weapon, kit)) {
			continue;
		}

		CAttributeList *const list{ weapon->m_AttributeList() };

		if (!list) {
			continue;
		}

		//integer attributes keep their bits in the float
		setAttribute(list, attributes::paintkit_proto_def_index, std::bit_cast<float>(kit));
		setAttribute(list, attributes::set_item_texture_wear, wearValue());
	}
}

void SkinChanger::run()
{
	if (!i::engine->IsInGame()) {
		return;
	}

	const bool changed{ m_applied_active != cfg::skins_active
		|| (cfg::skins_active && (m_applied[0] != cfg::skins_primary || m_applied[1] != cfg::skins_secondary || m_applied[2] != cfg::skins_melee || m_applied_wear != cfg::skins_wear)) };

	const float now{ i::global_vars->realtime };

	if (changed)
	{
		m_applied_active = cfg::skins_active;
		m_applied[0] = cfg::skins_primary;
		m_applied[1] = cfg::skins_secondary;
		m_applied[2] = cfg::skins_melee;
		m_applied_wear = cfg::skins_wear;

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
