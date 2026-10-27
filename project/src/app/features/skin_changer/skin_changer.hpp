#pragma once

#include "../../../game/game.hpp"

struct SkinOption final
{
	int id{};
	std::string name{};
};

struct SkinSlot final
{
	std::string weapon{}; //empty when nothing's equipped there or you're not in game
	std::vector<SkinOption> kits{};
};

//game side: what's equipped in a slot (0 primary, 1 secondary, 2 melee) and which paint kits it can take
SkinSlot skinSlot(const int slot);

//client side war paints on your own weapons, nobody else sees them
class SkinChanger final : public HasLevelInit
{
private:
	std::string m_applied{}; //every setting as text, a change means rebuilding the weapons
	float m_full_update_time{}; //settings changed, rebuild the weapons once they settle

public:
	bool onLevelInit() override;

public:
	//FRAME_NET_UPDATE_POSTDATAUPDATE_START, before freshly created weapons build their materials
	void apply();

	//FRAME_RENDER_START, notices changed settings and recreates the weapons so the new paint shows
	void run();
};

MAKE_UNIQUE(SkinChanger, skin_changer);
