#include "menu.hpp"
#include "widgets.hpp"
#include "ui.hpp"

#include "../statistics/statistics.hpp"

static const char *const class_names[stats_class_count]{ "", "scout", "sniper", "soldier", "demoman", "medic", "heavy", "pyro", "spy", "engineer" };

static double value(const StatBlock &block, const Stat stat)
{
	return block[static_cast<size_t>(stat)];
}

static std::string formatCount(const double v)
{
	//thousands separated, 1234567 -> 1,234,567
	std::string digits{ std::to_string(static_cast<long long>(std::llround(v))) };

	for (int i{ static_cast<int>(digits.size()) - 3 }; i > (digits[0] == '-' ? 1 : 0); i -= 3) {
		digits.insert(static_cast<size_t>(i), ",");
	}

	return digits;
}

static std::string formatTime(const double seconds)
{
	const long long total{ static_cast<long long>(seconds) };
	const long long h{ total / 3600 };
	const long long m{ (total % 3600) / 60 };
	const long long s{ total % 60 };

	if (h > 0) {
		return std::format("{}h {:02}m", h, m);
	}

	if (m > 0) {
		return std::format("{}m {:02}s", m, s);
	}

	return std::format("{}s", s);
}

static std::string formatStat(const Stat stat, const double v)
{
	const StatInfo &info{ statInfo(stat) };

	if (info.kind == StatKind::TIME || stat == Stat::LONGEST_LIFE) {
		return formatTime(v);
	}

	return formatCount(v);
}

static double ratio(const double a, const double b)
{
	return a / std::max(b, 1.0);
}

static std::string percent(const double part, const double whole)
{
	return whole > 0.0 ? std::format("{:.1f}%", part / whole * 100.0) : "-";
}

//label on the left, value on the right
static void statRow(const char *const label, const std::string &text)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(label);
	ImGui::TableNextColumn();

	const float w{ ImGui::CalcTextSize(text.c_str()).x };

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(ImGui::GetContentRegionAvail().x - w, 0.0f));
	ImGui::TextUnformatted(text.c_str());
}

static void statCard(const char *const id, const char *const title, const std::function<void()> &rows)
{
	ImGui::BeginChild(id, { scaled(200.0f), 0.0f }, ImGuiChildFlags_Border | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY);
	{
		ImGui::TextUnformatted(title);
		ImGui::Separator();

		if (ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp))
		{
			rows();
			ImGui::EndTable();
		}
	}
	ImGui::EndChild();
}

static void groupCard(const char *const id, const char *const title, const StatGroup group, const StatBlock &block)
{
	statCard(id, title, [&]()
	{
		for (size_t i{}; i < static_cast<size_t>(Stat::COUNT); i++)
		{
			const Stat stat{ static_cast<Stat>(i) };

			if (statInfo(stat).group == group) {
				statRow(statInfo(stat).label, formatStat(stat, block[i]));
			}
		}
	});
}

void Menu::tabStatistics()
{
	static int scope{}; //0 all time, 1 this session
	static std::string filter{}; //"" everything, "class:<n>" or "map:<name>"

	const StatsSnapshot snap{ statistics->snapshot() };

	//top bar
	{
		ImGui::SetNextItemWidth(scaled(110.0f));

		if (ImGui::BeginCombo("##stats_scope", scope == 0 ? "all time" : "this session"))
		{
			if (ImGui::Selectable("all time", scope == 0)) {
				scope = 0;
			}

			if (ImGui::Selectable("this session", scope == 1)) {
				scope = 1;
			}

			ImGui::EndCombo();
		}

		ImGui::SameLine();

		//class and map totals are only kept for all time
		ImGui::BeginDisabled(scope == 1);
		ImGui::SetNextItemWidth(scaled(150.0f));

		const std::string filter_label{ scope == 1 || filter.empty() ? "everything" : filter.starts_with("class:") ? class_names[std::stoi(filter.substr(6))] : filter.substr(4) };

		if (ImGui::BeginCombo("##stats_filter", filter_label.c_str(), ImGuiComboFlags_HeightLarge))
		{
			if (ImGui::Selectable("everything", filter.empty())) {
				filter.clear();
			}

			ImGui::SeparatorText("class");

			for (int c{ 1 }; c < stats_class_count; c++)
			{
				const std::string key{ std::format("class:{}", c) };

				if (ImGui::Selectable(class_names[c], filter == key)) {
					filter = key;
				}
			}

			if (!snap.per_map.empty())
			{
				ImGui::SeparatorText("map");

				for (const auto &[map, block] : snap.per_map)
				{
					const std::string key{ "map:" + map };

					if (ImGui::Selectable(map.c_str(), filter == key)) {
						filter = key;
					}
				}
			}

			ImGui::EndCombo();
		}

		ImGui::EndDisabled();

		ImGui::SameLine();
		//not a config setting, it lives with the stats
		if (bool active{ statistics->isActive() }; ImGui::Checkbox("track statistics", &active)) {
			statistics->setActive(active);
		}

		ImGui::SameLine();

		if (ImGui::Button("reset...")) {
			ImGui::OpenPopup("stats_reset_popup");
		}

		if (ImGui::BeginPopup("stats_reset_popup"))
		{
			if (ImGui::MenuItem("reset this session")) {
				confirm("reset this session's statistics?", []() { statistics->resetSession(); });
			}

			if (ImGui::MenuItem("reset everything")) {
				confirm("reset all statistics? this can't be undone", []() { statistics->resetAll(); });
			}

			ImGui::EndPopup();
		}
	}

	//the block everything below shows
	const StatBlock *block{ scope == 1 ? &snap.session : &snap.all_time };

	if (scope == 0 && filter.starts_with("class:")) {
		block = &snap.per_class[std::clamp(std::stoi(filter.substr(6)), 1, stats_class_count - 1)];
	}

	else if (scope == 0 && filter.starts_with("map:"))
	{
		const auto it{ snap.per_map.find(filter.substr(4)) };

		//a map that was reset away
		if (it == snap.per_map.end()) {
			filter.clear();
		}

		else {
			block = &it->second;
		}
	}

	const StatBlock &b{ *block };

	if (ImGui::BeginTabBar("tabbar_stats"))
	{
		StatsTab &tab{ m_stats_tab };

		if (ImGui::BeginTabItem("overview")) {
			tab = StatsTab::OVERVIEW;
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("classes")) {
			tab = StatsTab::CLASSES;
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("maps")) {
			tab = StatsTab::MAPS;
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("weapons")) {
			tab = StatsTab::WEAPONS;
			ImGui::EndTabItem();
		}

		beginSubTabPage();

		ImGui::BeginChild("group_stats", { 0.0f, -m_footer_h }, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

		if (tab == StatsTab::OVERVIEW)
		{
			const double kills{ value(b, Stat::KILLS) };
			const double deaths{ value(b, Stat::DEATHS) };
			const double assists{ value(b, Stat::ASSISTS) };
			const double hours{ value(b, Stat::TIME_IN_GAME) / 3600.0 };

			ImGui::BeginGroup();
			{
				statCard("stats_overview", "overview", [&]()
				{
					statRow("k/d", std::format("{:.2f}", ratio(kills, deaths)));
					statRow("kda", std::format("{:.2f}", ratio(kills + assists, deaths)));
					statRow("headshot %", percent(value(b, Stat::HEADSHOTS), kills));
					statRow("backstab %", percent(value(b, Stat::BACKSTABS), kills));
					statRow("kills per hour", hours > 0.0 ? std::format("{:.1f}", kills / hours) : "-");
					statRow("damage per minute", hours > 0.0 ? std::format("{:.0f}", value(b, Stat::DAMAGE_DEALT) / (hours * 60.0)) : "-");

					//favorites come from all time play, whatever's being looked at
					int fav_class{};

					for (int c{ 1 }; c < stats_class_count; c++)
					{
						if (value(snap.per_class[c], Stat::TIME_IN_GAME) > value(snap.per_class[fav_class], Stat::TIME_IN_GAME)) {
							fav_class = c;
						}
					}

					std::string fav_map{};
					double fav_map_time{};

					for (const auto &[map, map_block] : snap.per_map)
					{
						if (value(map_block, Stat::TIME_IN_GAME) > fav_map_time) {
							fav_map = map;
							fav_map_time = value(map_block, Stat::TIME_IN_GAME);
						}
					}

					statRow("favorite class", fav_class ? class_names[fav_class] : "-");
					statRow("favorite map", fav_map.empty() ? "-" : fav_map);
					statRow("current killstreak", formatCount(snap.current_streak));
					statRow("current life", formatTime(snap.current_life));
				});

				groupCard("stats_combat", "combat", StatGroup::COMBAT, b);
			}
			ImGui::EndGroup();

			ImGui::SameLine();

			ImGui::BeginGroup();
			{
				groupCard("stats_damage", "damage", StatGroup::DAMAGE, b);
				groupCard("stats_class", "class", StatGroup::CLASS, b);
			}
			ImGui::EndGroup();

			ImGui::SameLine();

			ImGui::BeginGroup();
			{
				groupCard("stats_objectives", "objectives", StatGroup::OBJECTIVES, b);
				groupCard("stats_records", "records", StatGroup::RECORDS, b);
				groupCard("stats_time", "time", StatGroup::TIME, b);
			}
			ImGui::EndGroup();
		}

		else
		{
			const ImGuiTableFlags flags{ ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp };

			ImGui::TextDisabled("all time");

			//sorted on whichever column header was clicked
			auto sortRows = [](auto &rows, const int column_count)
			{
				const ImGuiTableSortSpecs *const specs{ ImGui::TableGetSortSpecs() };

				if (!specs || specs->SpecsCount == 0) {
					return;
				}

				const int col{ std::clamp(static_cast<int>(specs->Specs[0].ColumnIndex), 0, column_count - 1) };
				const bool asc{ specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending };

				std::stable_sort(rows.begin(), rows.end(), [col, asc](const auto &a, const auto &b) {
					return asc ? a.second[col] < b.second[col] : a.second[col] > b.second[col];
				});
			};

			auto rightAligned = [](const std::string &text)
			{
				const float w{ ImGui::CalcTextSize(text.c_str()).x };

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(ImGui::GetContentRegionAvail().x - w, 0.0f));
				ImGui::TextUnformatted(text.c_str());
			};

			if (tab == StatsTab::CLASSES || tab == StatsTab::MAPS)
			{
				//name plus time, kills, deaths, assists, k/d, damage
				std::vector<std::pair<std::string, std::array<double, 7>>> rows{};

				auto addRow = [&rows](const std::string &name, const StatBlock &row)
				{
					const double k{ value(row, Stat::KILLS) };
					const double d{ value(row, Stat::DEATHS) };

					rows.push_back({ name, { 0.0, value(row, Stat::TIME_IN_GAME), k, d, value(row, Stat::ASSISTS), ratio(k, d), value(row, Stat::DAMAGE_DEALT) } });
				};

				if (tab == StatsTab::CLASSES)
				{
					for (int c{ 1 }; c < stats_class_count; c++) {
						addRow(class_names[c], snap.per_class[c]);
					}
				}

				else
				{
					for (const auto &[map, map_block] : snap.per_map) {
						addRow(map, map_block);
					}
				}

				if (rows.empty()) {
					ImGui::TextDisabled("nothing tracked yet");
				}

				else if (ImGui::BeginTable("stats_breakdown", 7, flags))
				{
					ImGui::TableSetupColumn(tab == StatsTab::CLASSES ? "class" : "map", ImGuiTableColumnFlags_NoSort, 2.0f);
					ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 1.2f);
					ImGui::TableSetupColumn("kills", ImGuiTableColumnFlags_PreferSortDescending, 1.0f);
					ImGui::TableSetupColumn("deaths", ImGuiTableColumnFlags_PreferSortDescending, 1.0f);
					ImGui::TableSetupColumn("assists", ImGuiTableColumnFlags_PreferSortDescending, 1.0f);
					ImGui::TableSetupColumn("k/d", ImGuiTableColumnFlags_PreferSortDescending, 0.8f);
					ImGui::TableSetupColumn("damage", ImGuiTableColumnFlags_PreferSortDescending, 1.2f);
					ImGui::TableHeadersRow();

					sortRows(rows, 7);

					for (const auto &[name, v] : rows)
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(name.c_str());

						ImGui::TableNextColumn();
						rightAligned(formatTime(v[1]));

						for (const int c : { 2, 3, 4 }) {
							ImGui::TableNextColumn();
							rightAligned(formatCount(v[c]));
						}

						ImGui::TableNextColumn();
						rightAligned(std::format("{:.2f}", v[5]));

						ImGui::TableNextColumn();
						rightAligned(formatCount(v[6]));
					}

					ImGui::EndTable();
				}
			}

			else if (tab == StatsTab::WEAPONS)
			{
				std::vector<std::pair<std::string, std::array<double, 2>>> rows{};

				//different items can share a name (every class's shotgun), those are one row
				std::map<std::string, double> by_name{};
				double total{};

				for (const auto &[key, kills] : snap.per_weapon) {
					by_name[weaponDisplayName(key)] += kills;
					total += kills;
				}

				for (const auto &[weapon, kills] : by_name) {
					rows.push_back({ weapon, { 0.0, kills } });
				}

				if (rows.empty()) {
					ImGui::TextDisabled("no kills tracked yet");
				}

				else if (ImGui::BeginTable("stats_weapons", 3, flags))
				{
					ImGui::TableSetupColumn("weapon", ImGuiTableColumnFlags_NoSort, 3.0f);
					ImGui::TableSetupColumn("kills", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 1.0f);
					ImGui::TableSetupColumn("share", ImGuiTableColumnFlags_NoSort, 1.0f);
					ImGui::TableHeadersRow();

					sortRows(rows, 2);

					for (const auto &[weapon, v] : rows)
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(weapon.c_str());

						ImGui::TableNextColumn();
						rightAligned(formatCount(v[1]));

						ImGui::TableNextColumn();
						rightAligned(percent(v[1], total));
					}

					ImGui::EndTable();
				}
			}
		}

		ImGui::EndChild();

		endSubTabPage();

		ImGui::EndTabBar();
	}
}
