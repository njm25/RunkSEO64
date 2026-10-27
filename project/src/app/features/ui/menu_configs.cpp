#include "menu.hpp"
#include "widgets.hpp"

#include "../notifs/notifs.hpp"
#include "../cfg.hpp"

void Menu::tabConfigs()
{
	static const std::string path{ utils::getFilePath() + "cfg\\" };

	if (!std::filesystem::exists(path)) {
		std::filesystem::create_directories(path);
	}

	static char new_cfg_name[50]{ '\0' };

	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

	if (ImGui::InputTextWithHint(" ", "create new", new_cfg_name, sizeof(new_cfg_name),
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank)) {
		cfg::save(std::format("{}{}.json", path, new_cfg_name));
		notifs->message(std::format("created '{}'", new_cfg_name));
		memset(new_cfg_name, 0, sizeof(new_cfg_name));
	}

	ImGui::PopItemWidth();

	if (!ImGui::IsItemActivated()) {
		memset(new_cfg_name, 0, sizeof(new_cfg_name));
	}

	ImGui::BeginChild("configs", ImVec2{ ImGui::GetContentRegionAvail().x, 0.0f },
					  ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
	{
		ImGui::Text("existing");

		ImGui::Separator();

		std::vector<std::filesystem::path> to_remove{};

		for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator{ path })
		{
			if (!file.path().extension().string().ends_with(".json")) {
				continue;
			}

			ImGui::PushID(file.path().string().c_str());

			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.0f, 0.5f });
			ImGui::Button(file.path().stem().string().c_str(), { floorf(ImGui::GetContentRegionAvail().x * 0.55f), 0.0f });
			ImGui::PopStyleVar();

			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.5f, 0.5f });

			ImGui::SameLine();

			const float w{ floorf(ImGui::GetContentRegionAvail().x * 0.3225f) };

			if (ImGui::Button("load", { w, 0.0f })) {
				cfg::load(file.path().string());
				notifs->message(std::string{ "loaded " } + std::format("'{}'", file.path().stem().string()));
			}

			ImGui::SameLine();

			if (ImGui::Button("save", { w, 0.0f })) {
				cfg::save(file.path().string());
				notifs->message(std::string{ "saved " } + std::format("'{}'", file.path().stem().string()));
			}

			ImGui::SameLine();

			if (ImGui::Button("delete", { w, 0.0f })) {
				to_remove.push_back(file.path());
				notifs->message(std::string{ "deleted " } + std::format("'{}'", file.path().stem().string()));
			}

			ImGui::PopStyleVar();

			ImGui::PopID();
		}

		for (const std::filesystem::path p : to_remove) {
			std::filesystem::remove(p);
		}
	}
	ImGui::EndChild();
}
