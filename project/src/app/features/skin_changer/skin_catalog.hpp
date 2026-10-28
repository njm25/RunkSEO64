#pragma once

#include "../../../game/game.hpp"

struct SkinStyle final
{
	int index{};
	std::string name{};
};

//one item definition that's a weapon, read from the game's own (prefab merged) schema
struct SkinItem final
{
	int def{};
	std::string name{}; //what the menu calls it
	std::string item_class{};
	std::string slot{};
	uint32_t classes{}; //1 << TF_CLASS_x
	int family{}; //index into SkinCatalog::families()

	bool stock{}; //baseitem, what everyone gets for free
	bool upgradeable{}; //the 19x-21x copies of the stock weapons that strange/australium/war paints go on
	bool festive{}; //a festive item, the lights are part of the item itself
	bool decorated{}; //15xxx, a war paint baked into its own item
	bool botkiller{};
	bool festivizable{}; //has lights for the festivizer
	bool australium{}; //has the gold style
	bool stattrak{}; //takes the strange counter module (everything war paintable does)

	std::vector<SkinStyle> styles{};
	std::vector<int> paintkits{};

	std::string extra_wearable{}; //botkiller head
	std::string extra_wearable_vm{};
	int skin_red{ -1 }; //botkiller tier colours, -1 means the model's default
	int skin_blu{ -1 };
};

//every variant of one weapon: same stats, different looks (stock, upgradeable, festive, botkillers, decorated, golden...)
struct SkinFamily final
{
	int base{}; //def the family is named and saved under
	std::string name{};
	std::string slot{};
	uint32_t classes{};
	std::vector<int> members{}; //sorted by def

	bool any_australium{};
	bool any_festivizable{};
	bool any_paintkits{};
	bool any_botkiller{};
};

class SkinCatalog final
{
private:
	std::vector<SkinItem> m_items{};
	std::vector<SkinFamily> m_families{};
	std::unordered_map<int, size_t> m_item_index{}; //def -> m_items
	int m_schema_version{ -1 };

private:
	void build(void *const schema);

public:
	//rebuilds when the game reloads its item schema, cheap otherwise
	bool ensure();

	const SkinItem *item(const int def) const;
	const SkinFamily *familyOf(const int def) const;
	const SkinFamily *familyByBase(const int base) const;
	const std::vector<SkinFamily> &families() const { return m_families; }

	//every family and its members to a text file, for checking the grouping
	std::string dump() const;
};

MAKE_UNIQUE(SkinCatalog, skin_catalog);
