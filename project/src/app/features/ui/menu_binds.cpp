#include "menu.hpp"
#include "widgets.hpp"
#include "binds.hpp"

void Menu::tabBinds()
{
	std::vector<Bind> &bind_list{ binds->getBinds() };

	ImGui::BeginChild("group_binds", { 0.0f, -m_footer_h });
	{
		if (bind_list.empty()) {
			ImGui::TextDisabled("no binds yet, right click a setting and pick bind as toggle or hold");
		}

		else
		{
			//one warning line per key that's bound more than once
			std::unordered_map<int, int> key_counts{};

			for (const Bind &bind : bind_list) {
				key_counts[bind.key]++;
			}

			for (const auto &[key, count] : key_counts)
			{
				if (key && count > 1) {
					ImGui::TextColored(warningColor(), "%s is bound %d times", utils::keyToStr(key).c_str(), count);
				}
			}

			const ImGuiTableFlags table_flags{ ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit };

			if (ImGui::BeginTable("binds_table", 8, table_flags))
			{
				const float h{ ImGui::GetFrameHeight() };

				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableSetupColumn("##drag", ImGuiTableColumnFlags_WidthFixed, h * 0.6f);
				ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthFixed, h * 3.5f);
				ImGui::TableSetupColumn("type", ImGuiTableColumnFlags_WidthFixed, h * 3.2f);
				ImGui::TableSetupColumn("off", ImGuiTableColumnFlags_WidthFixed, h * 5.0f);
				ImGui::TableSetupColumn("on", ImGuiTableColumnFlags_WidthFixed, h * 5.0f);
				ImGui::TableSetupColumn("list", ImGuiTableColumnFlags_WidthFixed, h);
				ImGui::TableSetupColumn("##remove", ImGuiTableColumnFlags_WidthFixed, h);
				ImGui::TableHeadersRow();

				//applied after the loop so the vector isn't changed while it's being drawn
				std::optional<std::pair<size_t, size_t>> move{};
				bool changed{};

				for (size_t i{}; i < bind_list.size(); i++)
				{
					Bind &bind{ bind_list[i] };

					ImGui::PushID(static_cast<int>(i));

					ImGui::TableNextRow();

					//drag handle, drop onto another row to move it there
					ImGui::TableNextColumn();
					ImGui::Selectable("::", false, ImGuiSelectableFlags_None, { 0.0f, h });

					if (ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload("BIND_REORDER", &i, sizeof(size_t));
						ImGui::Text("moving %s", bind.name.c_str());
						ImGui::EndDragDropSource();
					}

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload *const payload{ ImGui::AcceptDragDropPayload("BIND_REORDER") }) {
							move = { *static_cast<const size_t *>(payload->Data), i };
						}

						ImGui::EndDragDropTarget();
					}

					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(-FLT_MIN);

					inputString("##name", &bind.name);

					if (ImGui::IsItemDeactivatedAfterEdit()) {
						changed = true;
					}

					if (bind.var && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
						ImGui::SetTooltip("%s", bind.var->name().c_str());
					}

					ImGui::TableNextColumn();

					const std::string key_users{ bindKeyUsers(bind.key, &bind) };

					if (!key_users.empty()) {
						ImGui::PushStyleColor(ImGuiCol_Text, warningColor());
					}

					if (inputKey(&bind.key)) {
						changed = true;
					}

					if (!key_users.empty())
					{
						ImGui::PopStyleColor();

						if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
							ImGui::SetTooltip("also bound to: %s", key_users.c_str());
						}
					}

					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(-FLT_MIN);

					if (editBindType("##type", &bind.type)) {
						changed = true;
					}

					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(-FLT_MIN);

					if (editBindValue("off", bind, &bind.val_off)) {
						changed = true;
					}

					ImGui::TableNextColumn();
					ImGui::SetNextItemWidth(-FLT_MIN);

					if (editBindValue("on", bind, &bind.val_on)) {
						changed = true;
					}

					ImGui::TableNextColumn();

					if (ImGui::Checkbox("##list", &bind.appear_in_bindslist)) {
						changed = true;
					}

					if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
						ImGui::SetTooltip("show in the binds list while the menu is closed");
					}

					ImGui::TableNextColumn();

					if (ImGui::Button("x", { -FLT_MIN, 0.0f }))
					{
						confirm(std::format("remove the bind '{}'?", bind.name), [i, name = bind.name]()
						{
							//make sure the list didn't change underneath the prompt
							if (i < binds->getBinds().size() && binds->getBinds()[i].name == name) {
								binds->removeAt(i);
							}
						});
					}

					ImGui::PopID();
				}

				ImGui::EndTable();

				if (move) {
					binds->move(move->first, move->second);
				}

				else if (changed) {
					binds->save();
				}
			}
		}
	}
	ImGui::EndChild();
}
