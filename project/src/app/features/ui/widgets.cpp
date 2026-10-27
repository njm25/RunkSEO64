#include "widgets.hpp"
#include "binds.hpp"
#include "tooltips.hpp"
#include "ui.hpp"

#include "../notifs/notifs.hpp"
#include "../cfg.hpp"

#include <optional>
#include <type_traits>

enum class WidgetType
{
	TOGGLE, SELECT_SINGLE, SLIDER_INT, SLIDER_FLOAT
};

struct WidgetCtx
{
	WidgetType m_type{};

	std::any m_val_off{};
	std::any m_val_on{};

	WidgetCtx(const std::any val_off, const std::any val_on)
	{
		m_type = WidgetType::TOGGLE;
		m_val_off = val_off;
		m_val_on = val_on;
	}
};

struct WidgetCtxSelectSingle final : public WidgetCtx
{
	std::vector<std::pair<std::string, int>> m_vars{};

	WidgetCtxSelectSingle(const std::vector<std::pair<std::string, int>> &vars)
		: WidgetCtx(std::make_any<int>(vars.front().second), std::make_any<int>(vars.back().second))
	{
		m_type = WidgetType::SELECT_SINGLE;
		m_vars = vars;
	}
};

struct WidgetCtxSliderInt final : public WidgetCtx
{
	int m_val_min{};
	int m_val_max{};
	const char *m_fmt{};

	WidgetCtxSliderInt(const int min, const int max, const char *const fmt)
		: WidgetCtx(std::make_any<int>(min), std::make_any<int>(max))
	{
		m_type = WidgetType::SLIDER_INT;
		m_val_min = min;
		m_val_max = max;
		m_fmt = fmt;
	}
};

struct WidgetCtxSliderFloat final : public WidgetCtx
{
	float m_val_min{};
	float m_val_max{};
	const char *m_fmt{};

	WidgetCtxSliderFloat(const float min, const float max, const char *const fmt)
		: WidgetCtx(std::make_any<float>(min), std::make_any<float>(max))
	{
		m_type = WidgetType::SLIDER_FLOAT;
		m_val_min = min;
		m_val_max = max;
		m_fmt = fmt;
	}
};

//==================================================================================================================================


struct RegisteredWidget final
{
	std::string label{};
	ConfigVar *var{};
};

struct RegisteredSection final
{
	std::string key{};
	std::string title{};
	std::string path{}; //tab names leading to the section, e.g. "aimbot > main"
	std::vector<RegisteredWidget> widgets{};
};

//deque so pointers stay valid as sections get added
static std::deque<RegisteredSection> registered_sections{};
static RegisteredSection *cur_section{};
static std::string cur_page{};
static std::string cur_path{};

static WidgetMode mode{ WidgetMode::DRAW };
static std::vector<std::string> search_tokens{};
static bool search_show_all{}; //section itself matched, so every widget in it is shown
static bool search_goto{};
static int search_results{};

static RegisteredSection *findOrAddSection(const char *const id, const char *const title)
{
	const std::string key{ cur_page + "/" + id };

	for (RegisteredSection &sec : registered_sections)
	{
		if (sec.key == key) {
			return &sec;
		}
	}

	registered_sections.push_back(RegisteredSection{ key, title, cur_path });

	return &registered_sections.back();
}

//var is null for custom widgets that aren't backed by a cfg var, those are only there to be searchable
static void registerWidget(const char *const label, ConfigVar *const var)
{
	if (!cur_section) {
		return;
	}

	for (const RegisteredWidget &w : cur_section->widgets)
	{
		if (var ? w.var == var : (!w.var && w.label == label)) {
			return;
		}
	}

	cur_section->widgets.push_back(RegisteredWidget{ label, var });
}

static ConfigVar *findCfgVar(const void *const ptr)
{
	static std::unordered_map<const void *, ConfigVar *> cache{};

	if (cache.empty())
	{
		for (ConfigVar *const cfg_var : getInsts<ConfigVar>()) {
			cache[cfg_var->ptr()] = cfg_var;
		}
	}

	const auto it{ cache.find(ptr) };

	return it != cache.end() ? it->second : nullptr;
}

static std::string toLower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });

	return str;
}

//every word of the query has to show up somewhere in the haystack
static bool matchesSearch(const std::string &haystack)
{
	const std::string lower{ toLower(haystack) };

	for (const std::string &token : search_tokens)
	{
		if (lower.find(token) == std::string::npos) {
			return false;
		}
	}

	return true;
}

static std::string widgetHaystack(const RegisteredSection *const sec, const std::string &label, const ConfigVar *const var)
{
	std::string str{ label };

	if (sec) {
		str += " " + sec->path + " " + sec->title;
	}

	if (var) {
		str += " " + var->name();
	}

	return str;
}

//called at the top of every widget: registers it, and decides whether it gets drawn in the current mode
static bool widgetVisible(const char *const label, const void *const var)
{
	if (mode == WidgetMode::DRAW) {
		return true;
	}

	ConfigVar *const cfg_var{ findCfgVar(var) };

	if (mode == WidgetMode::REGISTER) {
		registerWidget(label, cfg_var);
		return false;
	}

	return search_show_all || matchesSearch(widgetHaystack(cur_section, label, cfg_var));
}

bool customWidget(const char *const label)
{
	return widgetVisible(label, nullptr);
}

void setWidgetMode(const WidgetMode new_mode)
{
	mode = new_mode;

	if (mode == WidgetMode::SEARCH) {
		search_results = 0;
		search_goto = false;
	}
}

WidgetMode widgetMode()
{
	return mode;
}

void setSearchQuery(std::string_view query)
{
	search_tokens.clear();

	std::string token{};

	for (const char c : toLower(std::string{ query }) + " ")
	{
		if (c != ' ') {
			token += c;
			continue;
		}

		if (!token.empty()) {
			search_tokens.push_back(token);
			token.clear();
		}
	}
}

void setSearchPath(std::string_view path)
{
	cur_path = path;
}

bool searchGotoClicked()
{
	return std::exchange(search_goto, false);
}

int searchResultCount()
{
	return search_results;
}

//==================================================================================================================================

static std::string confirm_msg{};
static std::function<void()> confirm_fn{};
static bool confirm_open{};

void confirm(std::string_view msg, const std::function<void()> &on_confirm)
{
	confirm_msg = msg;
	confirm_fn = on_confirm;
	confirm_open = true;
}

void drawConfirm()
{
	if (confirm_open) {
		ImGui::OpenPopup("##confirm");
		confirm_open = false;
	}

	if (ImGui::BeginPopupModal("##confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
	{
		//keys only count once the prompt is up, so the enter that opened it doesn't also confirm it
		const bool keys{ !ImGui::IsWindowAppearing() };

		ImGui::TextUnformatted(confirm_msg.c_str());

		ImGui::Separator();

		const float w{ ImGui::CalcTextSize("cancel").x + ImGui::GetStyle().FramePadding.x * 6.0f };

		if (ImGui::Button("yes", { w, 0.0f }) || (keys && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))))
		{
			if (confirm_fn) {
				confirm_fn();
			}

			confirm_fn = nullptr;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("cancel", { w, 0.0f }) || (keys && ImGui::IsKeyPressed(ImGuiKey_Escape, false))) {
			confirm_fn = nullptr;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

//==================================================================================================================================

static std::string formatValue(const ConfigVar *const var, const std::any &val, const WidgetCtx *const ctx)
{
	const size_t type{ var->hash() };

	if (type == typeid(bool).hash_code()) {
		return std::any_cast<bool>(val) ? "on" : "off";
	}

	else if (type == typeid(int).hash_code())
	{
		const int i{ std::any_cast<int>(val) };

		if (ctx && ctx->m_type == WidgetType::SELECT_SINGLE)
		{
			for (const std::pair<std::string, int> &v : static_cast<const WidgetCtxSelectSingle *>(ctx)->m_vars)
			{
				if (v.second == i) {
					return v.first;
				}
			}
		}

		if (ctx && ctx->m_type == WidgetType::SLIDER_INT)
		{
			char buf[64]{};
			snprintf(buf, sizeof(buf), static_cast<const WidgetCtxSliderInt *>(ctx)->m_fmt, i);
			return buf;
		}

		return std::to_string(i);
	}

	else if (type == typeid(float).hash_code())
	{
		const float f{ std::any_cast<float>(val) };

		char buf[64]{};
		snprintf(buf, sizeof(buf), ctx && ctx->m_type == WidgetType::SLIDER_FLOAT ? static_cast<const WidgetCtxSliderFloat *>(ctx)->m_fmt : "%.2f", f);
		return buf;
	}

	else if (type == typeid(std::string).hash_code())
	{
		const std::string str{ std::any_cast<std::string>(val) };
		return str.empty() ? "empty" : std::format("'{}'", str);
	}

	else if (type == typeid(Color).hash_code())
	{
		const Color clr{ std::any_cast<Color>(val) };
		return std::format("{}, {}, {}, {}", clr.r, clr.g, clr.b, clr.a);
	}

	return {};
}

//==================================================================================================================================

bool inputKey(int *const var, const bool start_listening)
{
	if (!var) {
		return false;
	}

	bool callback{};

	//listening state lives in imgui's storage so several key inputs can be on screen at once
	ImGuiStorage *const storage{ ImGui::GetStateStorage() };
	const ImGuiID state_id{ ImGui::GetID("##input_key_listening") };

	bool active{ start_listening || storage->GetBool(state_id) };

	if (!active)
	{
		if (!*var) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		}

		std::string str{ "bind key" };

		if (*var) {
			str = utils::keyToStr(*var);
		}

		const ImVec2 anchor{ ImGui::GetCursorPos() };

		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.0f, 0.5f });

		if (ImGui::Button(str.c_str(), ImVec2{ ImGui::GetContentRegionAvail().x, 0.0f })) {
			active = true;
			ImGui::SetCursorPos(anchor);
		}

		ImGui::PopStyleVar();

		if (!*var) {
			ImGui::PopStyleColor();
		}
	}

	if (active)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.0f, 0.5f });

		ImGui::Button("...", ImVec2{ ImGui::GetContentRegionAvail().x, 0.0f });

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		for (int n{}; n < 256; n++)
		{
			const bool mouse{ n > 0 && n < 7 };
			const bool letter{ n >= 'A' && n <= 'Z' };
			const bool num{ n >= 48 && n <= 57 };
			const bool esc{ n == VK_ESCAPE };
			const bool numpad{ n >= VK_NUMPAD0 && n <= VK_NUMPAD9 };
			const bool other{ n == VK_LSHIFT || n == VK_RSHIFT || n == VK_SHIFT || n == VK_INSERT || n == VK_MENU || n == VK_CAPITAL || n == VK_SPACE || n == VK_CONTROL };
			const bool allowed{ mouse || letter || num || esc || numpad || other };

			if (input_dx->getKey(n).pressed && allowed) {
				*var = (esc ? 0 : n);
				active = false;
				callback = true;
				break;
			}
		}
	}

	storage->SetBool(state_id, active);

	return callback;
}

static int resizeCallback(ImGuiInputTextCallbackData *data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		std::string *const str{ static_cast<std::string *>(data->UserData) };
		str->resize(data->BufTextLen);
		data->Buf = str->data();
	}

	return 0;
}

ImVec4 warningColor()
{
	const Color clr{ cfg::color_notifs_style_warning };

	return { clr.r / 255.0f, clr.g / 255.0f, clr.b / 255.0f, 1.0f };
}

bool editBindType(const char *const id, BindType *const type)
{
	bool changed{};

	if (ImGui::BeginCombo(id, *type == BindType::TOGGLE ? "toggle" : "hold"))
	{
		if (ImGui::Selectable("toggle", *type == BindType::TOGGLE)) {
			*type = BindType::TOGGLE;
			changed = true;
		}

		if (ImGui::Selectable("hold", *type == BindType::HOLD)) {
			*type = BindType::HOLD;
			changed = true;
		}

		ImGui::EndCombo();
	}

	return changed;
}

bool editBindValue(const char *const id, const Bind &bind, std::any *const val)
{
	if (!bind.var || !val) {
		return false;
	}

	bool changed{};

	const size_t type{ bind.var->hash() };

	ImGui::PushID(id);

	if (type == typeid(bool).hash_code())
	{
		const bool cur{ std::any_cast<bool>(*val) };

		if (ImGui::BeginCombo("##value", cur ? "true" : "false"))
		{
			for (const bool option : { false, true })
			{
				if (ImGui::Selectable(option ? "true" : "false", cur == option)) {
					*val = option;
					changed = true;
				}
			}

			ImGui::EndCombo();
		}
	}

	else if (type == typeid(int).hash_code() && !bind.selects.empty())
	{
		const int cur{ std::any_cast<int>(*val) };

		const char *preview{ "" };

		for (const std::pair<std::string, int> &select : bind.selects)
		{
			if (select.second == cur) {
				preview = select.first.c_str();
			}
		}

		if (ImGui::BeginCombo("##value", preview))
		{
			for (const std::pair<std::string, int> &select : bind.selects)
			{
				if (ImGui::Selectable(select.first.c_str(), select.second == cur)) {
					*val = select.second;
					changed = true;
				}
			}

			ImGui::EndCombo();
		}
	}

	//sliders update the value live but only count as changed once let go, so the binds file isn't written every frame of a drag
	else if (type == typeid(int).hash_code())
	{
		int cur{ std::any_cast<int>(*val) };

		const char *const fmt{ bind.fmt.empty() ? "%d" : bind.fmt.c_str() };

		const bool edited{ bind.has_range
			? ImGui::SliderInt("##value", &cur, static_cast<int>(bind.range_min), static_cast<int>(bind.range_max), fmt, ImGuiSliderFlags_AlwaysClamp)
			: ImGui::DragInt("##value", &cur, 0.1f, 0, 0, fmt) };

		if (edited) {
			*val = cur;
		}

		changed = ImGui::IsItemDeactivatedAfterEdit();
	}

	else if (type == typeid(float).hash_code())
	{
		float cur{ std::any_cast<float>(*val) };

		const char *const fmt{ bind.fmt.empty() ? "%.2f" : bind.fmt.c_str() };

		const bool edited{ bind.has_range
			? ImGui::SliderFloat("##value", &cur, bind.range_min, bind.range_max, fmt, ImGuiSliderFlags_AlwaysClamp)
			: ImGui::DragFloat("##value", &cur, 0.01f, 0.0f, 0.0f, fmt) };

		if (edited) {
			*val = cur;
		}

		changed = ImGui::IsItemDeactivatedAfterEdit();
	}

	ImGui::PopID();

	return changed;
}

std::string bindKeyUsers(const int key, const Bind *const ignore)
{
	std::string users{};

	if (!key) {
		return users;
	}

	for (const Bind &bind : binds->getBinds())
	{
		if (&bind == ignore || bind.key != key) {
			continue;
		}

		if (!users.empty()) {
			users += ", ";
		}

		users += bind.name;
	}

	return users;
}

static void bindPopup(void *const var, ConfigVar *const cfg_var, WidgetCtx *const ctx, const std::string &name, const std::optional<BindType> open_type)
{
	static Bind bind_data{};

	bool listen{};

	if (open_type)
	{
		ImGui::OpenPopup("bind_popup_bind");

		bind_data = Bind{};

		bind_data.name = name;
		bind_data.type = *open_type;
		bind_data.val_off = ctx->m_val_off;
		bind_data.val_on = ctx->m_val_on;
		bind_data.var = cfg_var;
		bind_data.cfg_var_name = cfg_var->name();
		bind_data.appear_in_bindslist = true;

		if (ctx->m_type == WidgetType::SELECT_SINGLE) {
			bind_data.selects = static_cast<WidgetCtxSelectSingle *>(ctx)->m_vars;
		}

		else if (ctx->m_type == WidgetType::SLIDER_INT)
		{
			const WidgetCtxSliderInt *const slider{ static_cast<WidgetCtxSliderInt *>(ctx) };

			bind_data.fmt = slider->m_fmt;
			bind_data.has_range = true;
			bind_data.range_min = static_cast<float>(slider->m_val_min);
			bind_data.range_max = static_cast<float>(slider->m_val_max);
		}

		else if (ctx->m_type == WidgetType::SLIDER_FLOAT)
		{
			const WidgetCtxSliderFloat *const slider{ static_cast<WidgetCtxSliderFloat *>(ctx) };

			bind_data.fmt = slider->m_fmt;
			bind_data.has_range = true;
			bind_data.range_min = slider->m_val_min;
			bind_data.range_max = slider->m_val_max;
		}

		//straight into waiting for a key
		listen = true;
	}

	if (ImGui::BeginPopup("bind_popup_bind"))
	{
		ImGui::TextUnformatted(std::format("binding {}", cfg_var->name()).c_str());

		ImGui::Separator();

		const float label_w{ ImGui::CalcTextSize("off").x + ImGui::GetStyle().ItemSpacing.x * 2.0f };

		ImGui::InputTextWithHint("##bind_name", "name", bind_data.name.data(), bind_data.name.capacity() + 1, ImGuiInputTextFlags_CallbackResize, resizeCallback, &bind_data.name);

		inputKey(&bind_data.key, listen);

		if (const std::string users{ bindKeyUsers(bind_data.key, nullptr) }; !users.empty()) {
			ImGui::TextColored(warningColor(), "also bound to: %s", users.c_str());
		}

		editBindType("##bind_type", &bind_data.type);

		ImGui::TextDisabled("off");
		ImGui::SameLine(label_w);
		editBindValue("val_off", bind_data, &bind_data.val_off);

		ImGui::TextDisabled("on");
		ImGui::SameLine(label_w);
		editBindValue("val_on", bind_data, &bind_data.val_on);

		ImGui::Separator();

		ImGui::Checkbox("show in binds", &bind_data.appear_in_bindslist);

		ImGui::Separator();

		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.0f, 0.5f });

		if (!bind_data.name.empty() && bind_data.key != 0)
		{
			if (ImGui::Button("save", ImVec2{ ImGui::GetContentRegionAvail().x, 0.0f }))
			{
				if (ctx->m_type == WidgetType::TOGGLE) {
					*static_cast<bool *>(var) = std::any_cast<bool>(bind_data.val_off);
				}

				else if (ctx->m_type == WidgetType::SELECT_SINGLE || ctx->m_type == WidgetType::SLIDER_INT) {
					*static_cast<int *>(var) = std::any_cast<int>(bind_data.val_off);
				}

				else if (ctx->m_type == WidgetType::SLIDER_FLOAT) {
					*static_cast<float *>(var) = std::any_cast<float>(bind_data.val_off);
				}

				binds->add(bind_data);

				ImGui::CloseCurrentPopup();
			}
		}

		else
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
			ImGui::Button("save", ImVec2{ ImGui::GetContentRegionAvail().x, 0.0f });
			ImGui::PopStyleColor();
		}

		ImGui::PopStyleVar();

		ImGui::EndPopup();
	}
}

static void widgetContext(const char *const label, void *const var, WidgetCtx *const ctx)
{
	if (!var) {
		return;
	}

	ConfigVar *const cfg_var{ findCfgVar(var) };

	if (!cfg_var) {
		return;
	}

	registerWidget(label, cfg_var);

	const char *const tooltip{ getTooltip(cfg_var->name()) };
	const bool slider{ ctx && (ctx->m_type == WidgetType::SLIDER_INT || ctx->m_type == WidgetType::SLIDER_FLOAT) };

	if ((tooltip || slider) && !ImGui::IsItemActive() && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 20.0f);

		if (tooltip) {
			ImGui::TextUnformatted(tooltip);
		}

		if (slider) {
			ImGui::TextDisabled("ctrl+click to type a value");
		}

		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}

	ImGui::PushID(var);

	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		ImGui::OpenPopup("widget_popup_context");
	}

	std::optional<BindType> open_bind{};

	if (ImGui::BeginPopup("widget_popup_context"))
	{
		ImGui::TextUnformatted(cfg_var->name().c_str());

		ImGui::Separator();

		if (ImGui::MenuItem(std::format("reset to default ({})", formatValue(cfg_var, cfg_var->defaultValue(), ctx)).c_str(), nullptr, false, !cfg_var->isDefault())) {
			cfg_var->reset();
		}

		if (ctx)
		{
			ImGui::Separator();

			if (ImGui::MenuItem("bind as toggle...")) {
				open_bind = BindType::TOGGLE;
			}

			if (ImGui::MenuItem("bind as hold...")) {
				open_bind = BindType::HOLD;
			}
		}

		ImGui::EndPopup();
	}

	if (ctx)
	{
		std::string name{};

		//"melee fov" reads better in the binds list than just "fov"
		if (open_bind)
		{
			name = std::string_view{ label }.substr(0, std::string_view{ label }.find("##"));

			if (cur_section) {
				name = cur_section->title + " " + name;
			}
		}

		bindPopup(var, cfg_var, ctx, name, open_bind);
	}

	ImGui::PopID();
}

//==================================================================================================================================

//scrolling over a hovered slider or combo nudges it, returns the notches scrolled (shift for 10x)
static float wheelNotches()
{
	if (!cfg::ui_scroll_edit || mode != WidgetMode::DRAW || ImGui::IsItemActive() || !ImGui::IsItemHovered()) {
		return 0.0f;
	}

	//claim the wheel while hovered so the page doesn't scroll along (shift+wheel scrolls sideways)
	ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
	ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelX);

	const ImGuiIO &io{ ImGui::GetIO() };

	return io.MouseWheel * (io.KeyShift ? 10.0f : 1.0f);
}

//smallest step the slider's format can show, "%.2f" -> 0.01
static float formatStep(const char *const fmt, const float min, const float max)
{
	if (const char *const dot{ fmt ? strstr(fmt, "%.") : nullptr }; dot && isdigit(static_cast<unsigned char>(dot[2]))) {
		return std::pow(10.0f, -static_cast<float>(atoi(dot + 2)));
	}

	return (max - min) / 100.0f;
}

static float stepValue(const float val, const float delta, const float step, const float min, const float max)
{
	//snapping to the step keeps repeated nudges from drifting
	return std::clamp(std::round((val + delta) / step) * step, min, max);
}

bool toggle(const char *const label, bool *const var)
{
	if (!widgetVisible(label, var)) {
		return false;
	}

	const bool callback{ ImGui::Checkbox(label, var) };

	WidgetCtx ctx{ std::make_any<bool>(false), std::make_any<bool>(true) };
	widgetContext(label, var, &ctx);

	return callback;
}

bool selectSingle(const char *const label, int *const var, std::vector<std::pair<std::string, int>> vars)
{
	if (!widgetVisible(label, var)) {
		return false;
	}

	bool callback{};

	if (ImGui::BeginCombo(label, vars[*var].first.c_str()))
	{
		for (int i{}; i < static_cast<int>(vars.size()); i++)
		{
			const bool selected{ i == *var };

			if (ImGui::Selectable(vars[i].first.c_str(), selected)) {
				*var = i;
				callback = true;
			}

			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

	if (const float notches{ wheelNotches() }; notches != 0.0f)
	{
		//scrolling up picks the previous option, like a list
		const int next{ std::clamp(*var + (notches > 0.0f ? -1 : 1), 0, static_cast<int>(vars.size()) - 1) };

		if (next != *var) {
			*var = next;
			callback = true;
		}
	}

	WidgetCtxSelectSingle ctx{ vars };
	widgetContext(label, var, &ctx);

	return callback;
}

bool selectMulti(const char *const label, const std::vector<std::pair<std::string, bool *>> vars)
{
	bool callback{};

	std::string preview{};

	for (const std::pair<std::string, bool *> &var : vars)
	{
		if (*var.second)
		{
			if (!preview.empty()) {
				preview += ", ";
			}

			preview += var.first;
		}
	}
	
	//items only get drawn while the combo is open, so register them up front
	bool any_visible{};

	for (const std::pair<std::string, bool *> &var : vars)
	{
		const std::string item_label{ std::format("{}: {}", label, var.first) };

		registerWidget(item_label.c_str(), findCfgVar(var.second));

		if (widgetVisible(item_label.c_str(), var.second)) {
			any_visible = true;
		}
	}

	if (!any_visible) {
		return false;
	}

	if (ImGui::BeginCombo(label, preview.c_str()))
	{
		for (const std::pair<std::string, bool *> &var : vars)
		{
			if (ImGui::SelectableWithValueOnRight(var.first.c_str(), false, std::format("{}", *var.second).c_str(), ImGuiSelectableFlags_NoAutoClosePopups)) {
				*var.second = !*var.second;
			}

			WidgetCtx ctx{ std::make_any<bool>(false), std::make_any<bool>(true) };
			widgetContext(std::format("{}: {}", label, var.first).c_str(), var.second, &ctx);
		}

		ImGui::EndCombo();
	}

	return callback;
}

//slider frame, then -/+ buttons, then the label, all in one group so hovering anywhere on the row counts as the widget
template <typename T>
static bool sliderRow(const char *const label, T *const var, const T min, const T max, const char *const fmt, WidgetCtx *const ctx)
{
	const ImGuiStyle &style{ ImGui::GetStyle() };
	const ImGuiIO &io{ ImGui::GetIO() };

	const float width{ ImGui::CalcItemWidth() };
	const float button{ ImGui::GetFrameHeight() };
	const bool buttons{ cfg::ui_slider_buttons };

	const std::string_view visible{ std::string_view{ label }.substr(0, std::string_view{ label }.find("##")) };
	const std::string slider_id{ std::format("##{}", label) };

	const float step{ std::is_same_v<T, int> ? 1.0f : formatStep(fmt, static_cast<float>(min), static_cast<float>(max)) };

	auto nudge = [&](const float steps) {
		*var = static_cast<T>(stepValue(static_cast<float>(*var), steps * step, step, static_cast<float>(min), static_cast<float>(max)));
	};

	bool callback{};

	ImGui::PushID(label);
	ImGui::BeginGroup();
	{
		ImGui::SetNextItemWidth(buttons ? std::max(width - (button + style.ItemInnerSpacing.x) * 2.0f, button) : width);

		if constexpr (std::is_same_v<T, int>) {
			callback = ImGui::SliderInt(slider_id.c_str(), var, min, max, fmt, ImGuiSliderFlags_AlwaysClamp);
		}

		else {
			callback = ImGui::SliderFloat(slider_id.c_str(), var, min, max, fmt, ImGuiSliderFlags_AlwaysClamp);
		}

		if (const float notches{ wheelNotches() }; notches != 0.0f) {
			nudge(notches);
			callback = true;
		}

		if (buttons)
		{
			//held buttons keep stepping
			ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);

			ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);

			if (ImGui::Button("-", { button, button })) {
				nudge(io.KeyShift ? -10.0f : -1.0f);
				callback = true;
			}

			ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);

			if (ImGui::Button("+", { button, button })) {
				nudge(io.KeyShift ? 10.0f : 1.0f);
				callback = true;
			}

			ImGui::PopItemFlag();
		}

		if (!visible.empty()) {
			ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
			ImGui::TextUnformatted(visible.data(), visible.data() + visible.size());
		}
	}
	ImGui::EndGroup();
	ImGui::PopID();

	widgetContext(label, var, ctx);

	return callback;
}

bool sliderInt(const char *const label, int *const var, int min, int max, const char *const fmt)
{
	if (!widgetVisible(label, var)) {
		return false;
	}

	WidgetCtxSliderInt ctx{ min, max, fmt };

	return sliderRow(label, var, min, max, fmt, &ctx);
}

bool sliderFloat(const char *const label, float *const var, float min, float max, const char *const fmt)
{
	if (!widgetVisible(label, var)) {
		return false;
	}

	WidgetCtxSliderFloat ctx{ min, max, fmt };

	return sliderRow(label, var, min, max, fmt, &ctx);
}

bool colorPicker(const char *const label, Color *const var)
{
	if (!widgetVisible(label, var)) {
		return false;
	}

	ImVec4 clr{};

	clr.x = static_cast<float>(var->r) / 255.0f;
	clr.y = static_cast<float>(var->g) / 255.0f;
	clr.z = static_cast<float>(var->b) / 255.0f;
	clr.w = static_cast<float>(var->a) / 255.0f;

	ImGuiColorEditFlags edit_flags{ ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar };

	//imgui's own options popup would take the right click from ours
	if (findCfgVar(var)) {
		edit_flags |= ImGuiColorEditFlags_NoOptions;
	}

	const bool callback{ ImGui::ColorEdit4(label, &clr.x, edit_flags) };

	if (callback)
	{
		var->r = static_cast<uint8_t>(clr.x * 255.0f + 0.5f);
		var->g = static_cast<uint8_t>(clr.y * 255.0f + 0.5f);
		var->b = static_cast<uint8_t>(clr.z * 255.0f + 0.5f);
		var->a = static_cast<uint8_t>(clr.w * 255.0f + 0.5f);
	}

	widgetContext(label, var, nullptr);

	return callback;
}

bool inputString(const char *const label, std::string *const var, const char *const fmt, int flags)
{
	if (!widgetVisible(label, var)) {
		return false;
	}

	const bool callback{ ImGui::InputText(label, var->data(), var->capacity() + 1, ImGuiInputTextFlags_CallbackResize | flags, resizeCallback, var) };

	widgetContext(label, var, nullptr);

	return callback;
}

bool inputStringWithHint(const char *const label, std::string *const var, const char *const hint, const char *const fmt, int flags)
{
	return ImGui::InputTextWithHint(label, hint, var->data(), var->capacity() + 1, ImGuiInputTextFlags_CallbackResize | flags, resizeCallback, var);
}

//==================================================================================================================================

//layout only exists while drawing, the other modes just walk the sections

void beginPage(const char *const id, const float bottom_padding)
{
	cur_page = id;

	//sections keep their width, so a narrow menu scrolls sideways instead of squashing them
	if (mode == WidgetMode::DRAW) {
		ImGui::BeginChild(id, { 0.0f, -bottom_padding }, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);
	}
}

void endPage()
{
	cur_page.clear();

	if (mode == WidgetMode::DRAW) {
		ImGui::EndChild();
	}
}

void beginColumn(const char *const id)
{
	if (mode == WidgetMode::DRAW) {
		ImGui::BeginChild(id, { 0.0f, 0.0f }, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
	}
}

void endColumn()
{
	if (mode == WidgetMode::DRAW) {
		ImGui::EndChild();
	}
}

void nextColumn()
{
	if (mode == WidgetMode::DRAW) {
		ImGui::SameLine();
	}
}

static void sectionContext(const RegisteredSection *const sec)
{
	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		ImGui::OpenPopup("section_popup_context");
	}

	if (ImGui::BeginPopup("section_popup_context"))
	{
		ImGui::TextUnformatted(sec->title.c_str());

		ImGui::Separator();

		const bool any_changed{ std::any_of(sec->widgets.begin(), sec->widgets.end(), [](const RegisteredWidget &w) { return w.var && !w.var->isDefault(); }) };

		if (ImGui::MenuItem("reset section to defaults", nullptr, false, any_changed))
		{
			confirm(std::format("reset all '{}' settings to their defaults?", sec->title), [sec]()
			{
				for (const RegisteredWidget &w : sec->widgets)
				{
					if (w.var) {
						w.var->reset();
					}
				}

				notifs->message(std::format("reset '{}' to defaults", sec->title));
			});
		}

		ImGui::EndPopup();
	}
}

void section(const char *const id, const char *const title, const float width, const std::function<void()> &body, const float item_width)
{
	ImGuiChildFlags flags{ ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY };

	//width of 0 fills the available space instead of fitting the contents
	if (width > 0.0f) {
		flags |= ImGuiChildFlags_AutoResizeX;
	}

	RegisteredSection *const sec{ findOrAddSection(id, title) };

	if (mode == WidgetMode::REGISTER)
	{
		cur_section = sec;
		body();
		cur_section = nullptr;

		return;
	}

	if (mode == WidgetMode::SEARCH)
	{
		const bool title_match{ matchesSearch(sec->path + " " + sec->title) };

		const bool any_match{ title_match || std::any_of(sec->widgets.begin(), sec->widgets.end(), [sec](const RegisteredWidget &w) {
			return matchesSearch(widgetHaystack(sec, w.label, w.var));
		}) };

		if (!any_match) {
			return;
		}

		search_results++;

		ImGui::BeginChild(sec->key.c_str(), { 0.0f, 0.0f }, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY);
		{
			ImGui::BeginGroup();
			ImGui::TextDisabled("%s >", sec->path.c_str());
			ImGui::SameLine();
			ImGui::TextUnformatted(title);
			ImGui::EndGroup();
			sectionContext(sec);

			const float goto_w{ ImGui::CalcTextSize("go to").x + ImGui::GetStyle().FramePadding.x * 2.0f };

			ImGui::SameLine(ImGui::GetContentRegionMax().x - goto_w);

			if (ImGui::SmallButton("go to")) {
				search_goto = true;
			}

			ImGui::Separator();

			ImGui::PushItemWidth(scaled(100.0f));

			cur_section = sec;
			search_show_all = title_match;
			body();
			search_show_all = false;
			cur_section = nullptr;

			ImGui::PopItemWidth();
		}
		ImGui::EndChild();

		return;
	}

	ImGui::BeginChild(id, { scaled(width), 0.0f }, flags);
	{
		ImGui::PushItemWidth(item_width > 0.0f ? scaled(item_width) : ImGui::GetContentRegionAvail().x);

		ImGui::TextUnformatted(title);
		sectionContext(sec);
		ImGui::Separator();

		cur_section = sec;
		body();
		cur_section = nullptr;

		ImGui::PopItemWidth();
	}
	ImGui::EndChild();
}

void sep()
{
	if (mode == WidgetMode::DRAW) {
		ImGui::Separator();
	}
}

void heading(const char *const text)
{
	if (mode == WidgetMode::DRAW) {
		ImGui::TextUnformatted(text);
	}
}
