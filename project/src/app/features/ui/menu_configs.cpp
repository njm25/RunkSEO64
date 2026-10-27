#include "menu.hpp"
#include "widgets.hpp"
#include "configs.hpp"

//stops characters windows won't accept in file names from being typed at all
static int configNameFilter(ImGuiInputTextCallbackData *data)
{
	if (data->EventChar < 32 || (data->EventChar < 128 && std::string_view{ "<>:\"/\\|?*" }.find(static_cast<char>(data->EventChar)) != std::string_view::npos)) {
		return 1;
	}

	return 0;
}

void Menu::tabConfigs()
{
	static char new_cfg_name[65]{ '\0' };

	const std::string name{ new_cfg_name };
	const std::string name_error{ Configs::validateName(name) };
	const bool name_exists{ name_error.empty() && configs->exists(name) };

	auto clearName = []() {
		memset(new_cfg_name, 0, sizeof(new_cfg_name));
	};

	auto create = [&]()
	{
		if (!name_error.empty()) {
			return;
		}

		if (name_exists) {
			confirm(std::format("'{}' already exists, overwrite it with the current settings?", name), [name]() { configs->save(name); });
		}

		else {
			configs->save(name);
		}

		clearName();
	};

	auto import = [&]()
	{
		if (!name_error.empty()) {
			return;
		}

		if (name_exists) {
			confirm(std::format("'{}' already exists, overwrite it with the config from the clipboard?", name), [name]() { configs->importFromClipboard(name); });
		}

		else if (configs->importFromClipboard(name)) {
			clearName();
		}
	};

	const ImGuiStyle &style{ ImGui::GetStyle() };
	const float button_w{ ImGui::CalcTextSize("import").x + style.FramePadding.x * 2.0f };

	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - (button_w + style.ItemSpacing.x) * 2.0f);

	ImGui::InputTextWithHint("##new_cfg", "config name", new_cfg_name, sizeof(new_cfg_name), ImGuiInputTextFlags_CallbackCharFilter, configNameFilter);

	if (ImGui::IsItemDeactivated() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))) {
		create();
	}

	ImGui::PopItemWidth();

	ImGui::BeginDisabled(!name_error.empty());
	{
		ImGui::SameLine();

		if (ImGui::Button("create", { button_w, 0.0f })) {
			create();
		}

		ImGui::SameLine();

		if (ImGui::Button("import", { button_w, 0.0f })) {
			import();
		}

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("create a config from one copied to the clipboard");
		}
	}
	ImGui::EndDisabled();

	//only complain once something's been typed
	if (!name.empty() && !name_error.empty()) {
		ImGui::TextColored(warningColor(), "%s", name_error.c_str());
	}

	else if (name_exists) {
		ImGui::TextColored(warningColor(), "'%s' already exists and will be overwritten", name.c_str());
	}

	ImGui::BeginChild("configs", { 0.0f, -m_footer_h }, ImGuiChildFlags_Border);
	{
		ImGui::Text("existing");

		ImGui::Separator();

		const bool dirty{ configs->hasUnsavedChanges() };

		for (const std::string &cfg_name : configs->list())
		{
			const bool is_active{ cfg_name == configs->active() };

			ImGui::PushID(cfg_name.c_str());

			if (is_active) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			}

			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.0f, 0.5f });
			ImGui::Button(is_active && dirty ? std::format("{} *", cfg_name).c_str() : cfg_name.c_str(), { floorf(ImGui::GetContentRegionAvail().x * 0.4f), 0.0f });
			ImGui::PopStyleVar();

			if (is_active)
			{
				ImGui::PopStyleColor(2);

				if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
					ImGui::SetTooltip(dirty ? "loaded, with unsaved changes" : "loaded");
				}
			}

			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.5f, 0.5f });

			ImGui::SameLine();

			const float w{ floorf((ImGui::GetContentRegionAvail().x - style.ItemSpacing.x * 3.0f) / 4.0f) };

			if (ImGui::Button("load", { w, 0.0f }))
			{
				if (dirty && !is_active) {
					confirm(std::format("discard unsaved changes to '{}' and load '{}'?", configs->active(), cfg_name), [cfg_name]() { configs->load(cfg_name); });
				}

				else if (dirty && is_active) {
					confirm(std::format("discard unsaved changes and reload '{}'?", cfg_name), [cfg_name]() { configs->load(cfg_name); });
				}

				else {
					configs->load(cfg_name);
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("save", { w, 0.0f })) {
				confirm(std::format("overwrite '{}' with the current settings?", cfg_name), [cfg_name]() { configs->save(cfg_name); });
			}

			ImGui::SameLine();

			if (ImGui::Button("copy", { w, 0.0f })) {
				configs->copyToClipboard(cfg_name);
			}

			if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
				ImGui::SetTooltip("copy to clipboard for sharing");
			}

			ImGui::SameLine();

			if (ImGui::Button("delete", { w, 0.0f })) {
				confirm(std::format("delete '{}'? this can't be undone", cfg_name), [cfg_name]() { configs->remove(cfg_name); });
			}

			ImGui::PopStyleVar();

			ImGui::PopID();
		}
	}
	ImGui::EndChild();
}
