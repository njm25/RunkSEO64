#pragma once

#include "skin_catalog.hpp"

//everything the skin changer puts on one weapon family, saved under the family's base def
struct WeaponOverride final
{
	bool enabled{ true };
	int variant{}; //member def to show as, 0 picks one that can show everything below
	int paintkit{};
	int wear{}; //0 factory new .. 4 battle scarred
	int seed{}; //war paint pattern, 0 leaves the paint's own
	bool australium{};
	bool festivized{};
	bool stattrak{};
	int ks_tier{}; //0 none, 1 basic, 2 specialized, 3 professional
	int sheen{}; //1-7, specialized and up
	int killstreaker{}; //1-7, professional
	int unusual{}; //0 none, 1 hot, 2 isotope, 3 cool, 4 energy orb
	int style{ -1 }; //-1 keeps the item's own
	int botkiller{}; //botkiller def to borrow the head from, 0 none
	bool spell_pumpkin{}; //halloween rocket trails, explosive weapons, only during halloween/full moon
	bool spell_green_flames{}; //flamethrowers, only during halloween/full moon

	bool operator==(const WeaponOverride &) const = default;
};

//which weapons the client draws each halloween spell for
bool canPumpkin(const SkinItem &item);
bool canGreenFlames(const SkinItem &item);

//a botkiller head we spawned ourselves, the server only spawns one for real botkillers
struct BotkillerHead final
{
	unsigned long weapon{}; //handles, they go stale instead of dangling
	unsigned long world{};
	unsigned long view{};
	unsigned long view_parent{}; //the viewmodel it follows, respawned if that ever changes
	int def{}; //botkiller it was made from
	int team{};

	//resolved every frame, ShouldDraw gets asked a lot
	uintptr_t world_ent{};
	uintptr_t view_ent{};
	bool world_drawn{};
	bool view_drawn{};
};

//client side looks for your own weapons, nobody else sees them
class SkinChanger final : public HasLevelInit, public HasLevelShutdown, public HasUnload
{
private:
	std::string m_applied{}; //every setting as text, a change means rebuilding the weapons
	float m_full_update_time{}; //settings changed, rebuild the weapons once they settle

	std::string m_parsed_from{}; //cfg::skins_overrides the map below was read from
	std::unordered_map<int, WeaponOverride> m_overrides{};

	std::vector<BotkillerHead> m_heads{};

private:
	void parse();
	void updateHeads();
	void removeHeads();

public:
	bool onLevelInit() override;
	bool onLevelShutdown() override;
	bool onUnload() override;

public:
	//FRAME_NET_UPDATE_POSTDATAUPDATE_START, before freshly created weapons build their looks
	void apply();

	//FRAME_RENDER_START, notices changed settings and recreates the weapons so the new looks show
	void run();

	//rebuild the weapons right away instead of after the settle delay
	void applyNow();

	//C_TFWeaponAttachmentModel::ShouldDraw for the heads we spawned, nullopt for the game's own attachments
	std::optional<bool> headShouldDraw(const uintptr_t ent) const;

public:
	const std::unordered_map<int, WeaponOverride> &overrides();
	const WeaponOverride *find(const int base);
	void set(const int base, const WeaponOverride &override);
	void erase(const int base);
	void clear();

	//the member a weapon gets shown as: the picked variant, else the current def if it can show everything, else the first member that can
	int resolveVariant(const SkinFamily &family, const WeaponOverride &override, const int current_def) const;

	//why a member can't show part of an override, empty when it can show all of it
	std::string unsupported(const SkinItem &item, const WeaponOverride &override) const;

	//families of the weapons you're holding right now, by base def
	std::vector<int> equippedFamilies() const;
};

MAKE_UNIQUE(SkinChanger, skin_changer);
