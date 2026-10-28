#include "menu.hpp"
#include "widgets.hpp"

#include "../diagnostics/diagnostics.hpp"
#include "../notifs/notifs.hpp"

#include <map>

static void summary(const std::vector<LoadItem> &items)
{
	struct Count final
	{
		size_t total{};
		size_t failed{};
	};

	std::map<LoadKind, Count> counts{};

	for (const LoadItem &item : items)
	{
		counts[item.kind].total++;

		if (!item.ok) {
			counts[item.kind].failed++;
		}
	}

	bool first{ true };

	for (const auto &[kind, count] : counts)
	{
		if (!first) {
			ImGui::SameLine();
			ImGui::TextDisabled("|");
			ImGui::SameLine();
		}

		first = false;

		const std::string text{ std::format("{} {}/{}", loadKindName(kind), count.total - count.failed, count.total) };

		if (count.failed) {
			ImGui::TextColored(warningColor(), "%s", text.c_str());
		}

		else {
			ImGui::TextUnformatted(text.c_str());
		}
	}

	if (diagnostics->gameUpdated())
	{
		std::string mods{};

		for (const std::string &mod : diagnostics->changedModules()) {
			mods += (mods.empty() ? "" : ", ") + mod;
		}

		ImGui::TextColored(warningColor(), "TF2 updated since the last load (%s)", mods.c_str());
	}
}

void Menu::tabDiagnostics()
{
	static bool failed_only{};
	static std::string filter{};

	const std::vector<LoadItem> items{ load_report->items() };

	summary(items);

	const ImGuiStyle &style{ ImGui::GetStyle() };
	const float copy_w{ ImGui::CalcTextSize("copy report").x + style.FramePadding.x * 2.0f };
	const float failed_w{ ImGui::GetFrameHeight() + style.ItemInnerSpacing.x + ImGui::CalcTextSize("failed only").x };

	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - copy_w - failed_w - style.ItemSpacing.x * 2.0f);
	inputStringWithHint("##diag_filter", &filter, "filter");

	ImGui::SameLine();
	ImGui::Checkbox("failed only", &failed_only);

	ImGui::SameLine();

	if (ImGui::Button("copy report", { copy_w, 0.0f })) {
		utils::copyToClipboard(diagnostics->report());
		notifs->message("copied the diagnostics report");
	}

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
		ImGui::SetTooltip("everything below as text, for a bug report");
	}

	ImGui::BeginChild("diagnostics", { 0.0f, -m_footer_h });
	{
		const std::unordered_map<std::string, std::string> &unavailable{ diagnostics->unavailable() };

		if (!unavailable.empty() && ImGui::CollapsingHeader(std::format("unavailable settings ({})###diag_unavailable", unavailable.size()).c_str()))
		{
			std::vector<std::pair<std::string, std::string>> sorted{ unavailable.begin(), unavailable.end() };
			std::sort(sorted.begin(), sorted.end());

			for (const auto &[var, reason] : sorted)
			{
				ImGui::BulletText("%s", var.c_str());

				if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
					ImGui::SetTooltip("%s", reason.c_str());
				}
			}
		}

		if (ImGui::CollapsingHeader("modules###diag_modules"))
		{
			for (const ModuleStamp &mod : diagnostics->modules())
			{
				const bool changed{ std::find(diagnostics->changedModules().begin(), diagnostics->changedModules().end(), mod.name) != diagnostics->changedModules().end() };

				const std::string text{ std::format("{}  timestamp 0x{:08X}  size 0x{:X}{}", mod.name, mod.timestamp, mod.size, changed ? "  (changed)" : "") };

				if (changed) {
					ImGui::TextColored(warningColor(), "%s", text.c_str());
				}

				else {
					ImGui::TextUnformatted(text.c_str());
				}
			}
		}

		const ImGuiTableFlags table_flags{ ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable };

		if (ImGui::BeginTable("diag_table", 5, table_flags))
		{
			ImGui::TableSetupColumn("kind");
			ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("status");
			ImGui::TableSetupColumn("address");
			ImGui::TableSetupColumn("time");
			ImGui::TableHeadersRow();

			std::string lower_filter{ filter };
			std::transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });

			//failures first
			for (const bool ok : { false, true })
			{
				if (ok && failed_only) {
					break;
				}

				for (const LoadItem &item : items)
				{
					if (item.ok != ok) {
						continue;
					}

					if (!lower_filter.empty())
					{
						std::string name{ std::format("{} {}", loadKindName(item.kind), item.name) };
						std::transform(name.begin(), name.end(), name.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });

						if (name.find(lower_filter) == std::string::npos) {
							continue;
						}
					}

					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::TextDisabled("%s", loadKindName(item.kind));

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(item.name.c_str());

					ImGui::TableNextColumn();

					if (item.ok) {
						ImGui::TextUnformatted("ok");
					}

					else {
						ImGui::TextColored(warningColor(), "failed");
					}

					if (!item.detail.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
						ImGui::SetTooltip("%s", item.detail.c_str());
					}

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(diagnostics->formatAddress(item).c_str());

					ImGui::TableNextColumn();

					if (item.ms > 0.0) {
						ImGui::Text("%.2f ms", item.ms);
					}

					else {
						ImGui::TextDisabled("-");
					}
				}
			}

			ImGui::EndTable();
		}
	}
	ImGui::EndChild();
}
