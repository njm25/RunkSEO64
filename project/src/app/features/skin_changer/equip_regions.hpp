#pragma once

#include <cstdint>
#include <string_view>

//items_game.txt "equip_regions_list" (line 245) and "equip_conflicts" (line 338).
//kept here instead of read off the item definitions, misc_remove_equip_regions zeroes the game's masks for good
namespace equip_regions
{
	struct Region final
	{
		const char *name{};
		int bit{};
	};

	//the "shared" blocks share one bit, they're one per class so they never meet on one player
	inline constexpr Region regions[]
	{
		{ "whole_head", 0 }, { "hat", 1 }, { "face", 2 }, { "glasses", 3 }, { "lenses", 4 }, { "pants", 5 },
		{ "beard", 6 }, { "shirt", 7 }, { "medal", 8 }, { "arms", 9 }, { "back", 10 }, { "feet", 11 },
		{ "necklace", 12 }, { "grenades", 13 }, { "arm_tattoos", 14 }, { "flair", 15 }, { "head_skin", 16 }, { "ears", 17 },
		{ "left_shoulder", 18 }, { "belt_misc", 19 }, { "disconnected_floating_item", 20 }, { "zombie_body", 21 }, { "sleeves", 22 }, { "right_shoulder", 23 },

		{ "pyro_spikes", 24 }, { "scout_bandages", 24 }, { "engineer_pocket", 24 }, { "heavy_belt_back", 24 }, { "demo_eyepatch", 24 }, { "soldier_gloves", 24 }, { "spy_gloves", 24 },
		{ "scout_backpack", 25 }, { "heavy_pocket", 25 }, { "engineer_belt", 25 }, { "soldier_pocket", 25 }, { "demo_belt", 25 }, { "sniper_quiver", 25 },
		{ "pyro_wings", 26 }, { "sniper_bullets", 26 }, { "medigun_accessories", 26 }, { "soldier_coat", 26 }, { "heavy_belt", 26 }, { "scout_hands", 26 },
		{ "engineer_left_arm", 27 }, { "pyro_tail", 27 }, { "sniper_legs", 27 }, { "medic_gloves", 27 }, { "soldier_cigar", 27 }, { "demoman_collar", 27 }, { "heavy_towel", 27 },
		{ "engineer_wings", 28 }, { "pyro_head_replacement", 28 }, { "scout_wings", 28 }, { "heavy_hair", 28 }, { "medic_pipe", 28 }, { "soldier_legs", 28 }, { "demo_head_replacement", 28 }, { "sniper_headband", 28 },
		{ "scout_pants", 29 }, { "heavy_bullets", 29 }, { "engineer_hair", 29 }, { "sniper_vest", 29 }, { "medigun_backpack", 29 }, { "sniper_pocket_left", 29 },
		{ "sniper_pocket", 30 }, { "heavy_hip", 30 }, { "spy_coat", 30 }, { "medic_hip", 30 }
	};

	//"glasses" { "face" "lenses" }, "whole_head" { "hat" "face" "glasses" }
	struct Conflict final
	{
		int bit{};
		uint32_t also{};
	};

	inline constexpr Conflict conflicts[]
	{
		{ 3, (1u << 2) | (1u << 4) },
		{ 0, (1u << 1) | (1u << 2) | (1u << 3) }
	};

	inline int bitOf(std::string_view name)
	{
		for (const Region &region : regions)
		{
			if (name == region.name) {
				return region.bit;
			}
		}

		return -1;
	}

	//the region's own bits plus whatever they rule out
	inline uint32_t expand(const uint32_t own)
	{
		uint32_t mask{ own };

		for (const Conflict &conflict : conflicts)
		{
			if (own & (1u << conflict.bit)) {
				mask |= conflict.also;
			}
		}

		return mask;
	}
}
