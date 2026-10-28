#include "menu.hpp"
#include "widgets.hpp"
#include "ui.hpp"

#include "../player_list/player_list.hpp"

void Menu::tabPlayers()
{
	ImGui::BeginChild("group_player_list", { 0.0f, -m_footer_h });
	{
		ImGui::BeginChild("group_player_list_tags", { scaled(200.0f), 0.0f }, ImGuiChildFlags_Border);
		{
			ImGui::PushItemWidth(scaled(100.0f));

			ImGui::Text("tags");
			ImGui::Separator();

			static std::string new_tag_name{};

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

			if (inputStringWithHint("##new_tag", &new_tag_name, "create new", "%s", ImGuiInputTextFlags_EnterReturnsTrue)) {
				PlayerListTag tag{ new_tag_name, Color{ 255,255,255,255 }, 0 };
				player_list->add(tag);
				new_tag_name.clear();
			}

			ImGui::PopItemWidth();

			ImGui::Separator();

			ImGui::BeginChild("group_player_list_existing_tags", { 0.0f, 0.0f });
			{
				const float frame_padding{ ImGui::GetStyle().FramePadding.x };

				for (const auto &[tag_name, tag_hash] : player_list->getTags())
				{
					ImGui::PushID(tag_hash);

					PlayerListTag *const tag{ player_list->findTagByHash(tag_hash) };

					// how?
					if (!tag) {
						ImGui::PopID();
						continue;
					}

					ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.0f, 0.5f });
					ImGui::Button(tag_name.c_str(), { ImGui::GetContentRegionAvail().x, 0.0f });
					ImGui::PopStyleVar();

					if (ImGui::IsItemHovered() && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
						ImGui::OpenPopup("tag_popup_context");
					}

					bool edit_tag{};

					if (ImGui::BeginPopup("tag_popup_context"))
					{
						ImGui::Text("tag options");

						ImGui::Separator();

						if (ImGui::MenuItem("edit")) {
							edit_tag = true;
						}

						if (ImGui::MenuItem("delete")) {
							player_list->remove(*tag);
						}

						ImGui::EndPopup();
					}

					if (edit_tag) {
						ImGui::OpenPopup("tag_popup_edit");
					}

					if (ImGui::BeginPopup("tag_popup_edit"))
					{
						ImGui::PushItemWidth(scaled(100.0f));
						ImGui::Text("edit tag");

						ImGui::Separator();

						bool save{};

						if (inputString("name", &tag->name, "%s", ImGuiInputTextFlags_EnterReturnsTrue)) {
							save = true;
						}

						if (ImGui::SliderInt("priority", &tag->priority, -1, 10, tag->priority == -1 ? "ignored" : "%d")) {
							save = true;
						}

						if (colorPicker("color", &tag->color)) {
							save = true;
						}

						if (save) {
							player_list->save(utils::getFilePath() + "player_list.json");
						}

						ImGui::PopItemWidth();
						ImGui::EndPopup();
					}


					ImGui::PopID();
				}
			}
			ImGui::EndChild();

			ImGui::PopItemWidth();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("group_player_list_players", { 0.0f, 0.0f }, ImGuiChildFlags_Border);
		{
			ImGui::PushItemWidth(scaled(100.0f));

			ImGui::Text("players");
			ImGui::Separator();

			static char searched_player[32]{ '\0' };

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::InputTextWithHint(" ", "search for player", searched_player, sizeof(searched_player), ImGuiInputTextFlags_AutoSelectAll);
			ImGui::PopItemWidth();

			ImGui::Separator();

			for (int i{}; i < i::ent_list->GetHighestEntityIndex(); i++)
			{
				IClientEntity *const entity{ i::ent_list->GetClientEntity(i) };

				if (!entity) {
					continue;
				}

				if (entity->GetClassId() != class_ids::CTFPlayer) {
					continue;
				}

				C_TFPlayer *player{ static_cast<C_TFPlayer *>(entity) };

				player_info_t player_info{};

				if (!i::engine->GetPlayerInfo(player->entindex(), &player_info) || player_info.fakeplayer) {
					continue;
				}

				const std::string player_name{ utils::wideToUtf8(utils::utf8ToWide(player_info.name)) };

				if (std::string cmp{ searched_player }; cmp.size())
				{
					std::string transform{ player_name };

                                std::transform(transform.begin(), transform.end(), transform.begin(), ::tolower);
					std::transform(cmp.begin(), cmp.end(), cmp.begin(), ::tolower);

					if (transform.find(cmp) == std::string::npos) {
						continue;
					}
				}

				ImGui::PushID(player_info.friendsID);

				ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.0f, 0.5f });
				ImGui::Button(player_name.c_str(), { ImGui::GetContentRegionAvail().x, 0.0f });
				ImGui::PopStyleVar();

				if (ImGui::IsItemHovered() && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
					ImGui::OpenPopup("player_popup_context");
				}

				bool edit_player{};

				if (ImGui::BeginPopup("player_popup_context"))
				{
					ImGui::Text("player options");

					ImGui::Separator();

					if (ImGui::MenuItem("add to tag")) {
						edit_player = true;
					}

					if (ImGui::MenuItem("remove from tag")) {
						player_list->setPlayerTag(player_info.friendsID, 0);
					}

					if (ImGui::MenuItem("votekick")) {
						i::engine->ClientCmd_Unrestricted(std::format("callvote kick {}", player_info.userID).c_str());
					}

					if (ImGui::MenuItem("steam profile"))
					{
						i::steam_api_context->SteamFriends()->ActivateGameOverlayToToUser("steamid", CSteamID(
							player_info.friendsID,
							k_EUniversePublic,
							k_EAccountTypeIndividual
						));
					}

					if (ImGui::MenuItem("steam history")) {
						CSteamID id(player_info.friendsID, k_EUniversePublic, k_EAccountTypeIndividual);
						i::steam_api_context->SteamFriends()->ActivateGameOverlayToWebPage(std::format("https://steamhistory.net/id/{}", id.ConvertToUint64()).c_str());
					}

					ImGui::EndPopup();
				}

				if (edit_player) {
					ImGui::OpenPopup("player_popup_add_to_tag");
				}

				if (ImGui::BeginPopup("player_popup_add_to_tag"))
				{
					ImGui::Text("add to tag");

					ImGui::Separator();

					PlayerListTag *existing_tag{ player_list->findPlayer(player) };

					for (const auto &[tag_name, tag_hash] : player_list->getTags())
					{
						PlayerListTag *const tag{ player_list->findTagByHash(tag_hash) };

						if (!tag || tag == existing_tag) {
							continue;
						}

						if (ImGui::MenuItem(tag_name.c_str())) {
							player_list->setPlayerTag(player_info.friendsID, tag_hash);
						}
					}

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}

			ImGui::PopItemWidth();
		}
		ImGui::EndChild();
	}
	ImGui::EndChild();
}
