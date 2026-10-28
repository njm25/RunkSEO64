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

//a hat or misc item a class can put in one of its three cosmetic slots
struct SkinCosmetic final
{
	int def{};
	std::string name{};
	std::string slot{}; //head or misc, only a category, any cosmetic fits any slot
	uint32_t classes{};

	uint32_t own{}; //equip region bits it takes
	uint32_t mask{}; //and the ones it rules out
	std::vector<std::string> regions{}; //names, for the menu and for regions the table doesn't know

	bool paintable{};
	bool hides_bodygroups{}; //hides part of the player model (hat hair, headphones...)
	std::vector<std::string> bodygroups{}; //the player model's bodygroups it hides
	std::vector<SkinStyle> styles{};
};

struct SkinPaint final
{
	const char *name{};
	int red{}; //"set item tint RGB", the only colour of a plain paint
	int blu{}; //"set item tint RGB 2", team paints only
};

struct SkinUnusual final
{
	int id{};
	std::string name{};
};

class SkinCatalog final
{
private:
	std::vector<SkinItem> m_items{};
	std::vector<SkinFamily> m_families{};
	std::unordered_map<int, size_t> m_item_index{}; //def -> m_items
	int m_schema_version{ -1 };

	std::vector<SkinCosmetic> m_cosmetics{}; //sorted by name
	std::unordered_map<int, size_t> m_cosmetic_index{};
	std::vector<SkinUnusual> m_unusuals{}; //sorted by name

private:
	void build(void *const schema);
	void buildUnusuals();

public:
	//rebuilds when the game reloads its item schema, cheap otherwise
	bool ensure();

	const SkinItem *item(const int def) const;
	const SkinFamily *familyOf(const int def) const;
	const SkinFamily *familyByBase(const int base) const;
	const std::vector<SkinFamily> &families() const { return m_families; }

	const std::vector<SkinCosmetic> &cosmetics() const { return m_cosmetics; }
	const SkinCosmetic *cosmetic(const int def) const;

	//the region two cosmetics clash on, empty when they can be worn together
	std::string conflict(const SkinCosmetic &a, const SkinCosmetic &b) const;

	//cosmetic unusual effects with an english name, the blu half of team coloured pairs left out
	const std::vector<SkinUnusual> &unusuals() const { return m_unusuals; }

	//the blu version of a team coloured effect, the same id for the rest
	static int unusualForTeam(const int id, const int team);

	static const std::vector<SkinPaint> &paints();

	//every family and its members to a text file, for checking the grouping
	std::string dump() const;
};

//english text for a "#Token" (or a bare token), empty when the game has none
std::string localize(std::string_view token);

MAKE_UNIQUE(SkinCatalog, skin_catalog);
