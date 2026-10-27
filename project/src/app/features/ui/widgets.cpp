#include "widgets.hpp"
#include "binds.hpp"

#include "../notifs/notifs.hpp"

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
	std::vector<RegisteredWidget> widgets{};
};

//deque so pointers stay valid as sections get added
static std::deque<RegisteredSection> registered_sections{};
static RegisteredSection *cur_section{};
static std::string cur_page{};

static RegisteredSection *findOrAddSection(const char *const id, const char *const title)
{
	const std::string key{ cur_page + "/" + id };

	for (RegisteredSection &sec : registered_sections)
	{
		if (sec.key == key) {
			return &sec;
		}
	}

	registered_sections.push_back(RegisteredSection{ key, title });

	return &registered_sections.back();
}

static void registerWidget(const char *const label, ConfigVar *const var)
{
	if (!cur_section || !var) {
		return;
	}

	for (const RegisteredWidget &w : cur_section->widgets)
	{
		if (w.var == var) {
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

bool inputKey(int *const var)
{
	if (!var) {
		return false;
	}

	bool callback{};

	static bool active{};

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

static void bindPopup(void *const var, ConfigVar *const cfg_var, WidgetCtx *const ctx, const bool open_bind)
{
	static Bind bind_data{};

	if (open_bind)
	{
		ImGui::OpenPopup("bind_popup_bind");

		bind_data = Bind{};

		bind_data.val_off = ctx->m_val_off;
		bind_data.val_on = ctx->m_val_on;
		bind_data.var = cfg_var;
		bind_data.cfg_var_name = cfg_var->name();
		bind_data.appear_in_bindslist = true;
	}

	if (ImGui::BeginPopup("bind_popup_bind"))
	{
		ImGui::Text(std::format("binding {}", cfg_var->name()).c_str());

		ImGui::Separator();

		ImGui::InputTextWithHint("##bind_name", "name", bind_data.name.data(), bind_data.name.capacity() + 1, ImGuiInputTextFlags_CallbackResize, resizeCallback, &bind_data.name);

		inputKey(&bind_data.key);

		if (ImGui::BeginCombo("##bind_type", bind_data.type == BindType::TOGGLE ? "toggle" : "hold"))
		{
			if (ImGui::Selectable("toggle", bind_data.type == BindType::TOGGLE)) {
				bind_data.type = BindType::TOGGLE;
			}

			if (ImGui::Selectable("hold", bind_data.type == BindType::HOLD)) {
				bind_data.type = BindType::HOLD;
			}

			ImGui::EndCombo();
		}

		if (ctx->m_type == WidgetType::TOGGLE)
		{
			if (ImGui::BeginCombo("##val_off", std::any_cast<bool>(bind_data.val_off) ? "true" : "false"))
			{
				if (ImGui::Selectable("false", std::any_cast<bool>(bind_data.val_off) == false)) {
					bind_data.val_off = std::make_any<bool>(false);
				}

				if (ImGui::Selectable("true", std::any_cast<bool>(bind_data.val_off) == true)) {
					bind_data.val_off = std::make_any<bool>(true);
				}

				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("##val_on", std::any_cast<bool>(bind_data.val_on) ? "true" : "false"))
			{
				if (ImGui::Selectable("false", std::any_cast<bool>(bind_data.val_on) == false)) {
					bind_data.val_on = std::make_any<bool>(false);
				}

				if (ImGui::Selectable("true", std::any_cast<bool>(bind_data.val_on) == true)) {
					bind_data.val_on = std::make_any<bool>(true);
				}

				ImGui::EndCombo();
			}
		}

		else if (ctx->m_type == WidgetType::SELECT_SINGLE)
		{
			WidgetCtxSelectSingle *const select{ static_cast<WidgetCtxSelectSingle *>(ctx) };

			auto valToSelect = [&](const int val)
			{
				for (const std::pair<std::string, int> &v : select->m_vars)
				{
					if (v.second != val) {
						continue;
					}

					return v;
				}

				return std::pair<std::string, int>{};
			};

			const std::pair<std::string, int> cur_off{ valToSelect(std::any_cast<int>(bind_data.val_off)) };
			const std::pair<std::string, int> cur_on{ valToSelect(std::any_cast<int>(bind_data.val_on)) };

			if (ImGui::BeginCombo("##val_off", cur_off.first.c_str()))
			{
				for (const std::pair<std::string, int> &v : select->m_vars)
				{
					if (ImGui::Selectable(v.first.c_str(), std::any_cast<int>(bind_data.val_off) == v.second)) {
						bind_data.val_off = std::make_any<int>(v.second);
					}
				}

				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("##val_on", cur_on.first.c_str()))
			{
				for (const std::pair<std::string, int> &v : select->m_vars)
				{
					if (ImGui::Selectable(v.first.c_str(), std::any_cast<int>(bind_data.val_on) == v.second)) {
						bind_data.val_on = std::make_any<int>(v.second);
					}
				}

				ImGui::EndCombo();
			}
		}

		else if (ctx->m_type == WidgetType::SLIDER_INT)
		{
			WidgetCtxSliderInt *const slider{ static_cast<WidgetCtxSliderInt *>(ctx) };

			static int slider_off{};
			static int slider_on{};

			slider_off = std::any_cast<int>(bind_data.val_off);
			slider_on = std::any_cast<int>(bind_data.val_on);

			if (ImGui::SliderInt("##val_off", &slider_off, slider->m_val_min, slider->m_val_max, slider->m_fmt, ImGuiSliderFlags_AlwaysClamp)) {
				bind_data.val_off = std::make_any<int>(slider_off);
			}

			if (ImGui::SliderInt("##val_on", &slider_on, slider->m_val_min, slider->m_val_max, slider->m_fmt, ImGuiSliderFlags_AlwaysClamp)) {
				bind_data.val_on = std::make_any<int>(slider_on);
			}
		}

		else if (ctx->m_type == WidgetType::SLIDER_FLOAT)
		{
			WidgetCtxSliderFloat *const slider{ static_cast<WidgetCtxSliderFloat *>(ctx) };

			static float slider_off{};
			static float slider_on{};

			slider_off = std::any_cast<float>(bind_data.val_off);
			slider_on = std::any_cast<float>(bind_data.val_on);

			if (ImGui::SliderFloat("##val_off", &slider_off, slider->m_val_min, slider->m_val_max, slider->m_fmt, ImGuiSliderFlags_AlwaysClamp)) {
				bind_data.val_off = std::make_any<float>(slider_off);
			}

			if (ImGui::SliderFloat("##val_on", &slider_on, slider->m_val_min, slider->m_val_max, slider->m_fmt, ImGuiSliderFlags_AlwaysClamp)) {
				bind_data.val_on = std::make_any<float>(slider_on);
			}
		}

		ImGui::Separator();

		ImGui::Checkbox("show in binds", &bind_data.appear_in_bindslist);

		ImGui::Separator();

		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2{ 0.0f, 0.5f });

		if (!std::string_view{ bind_data.name.data() }.empty() && bind_data.key != 0)
		{
			if (ImGui::Button("save", ImVec2{ ImGui::GetContentRegionAvail().x, 0.0f }))
			{
				if (ctx->m_type == WidgetType::TOGGLE) {
					*static_cast<bool *>(var) = std::any_cast<bool>(bind_data.val_off);
				}

				else if (ctx->m_type == WidgetType::SELECT_SINGLE) {
					*static_cast<int *>(var) = std::any_cast<int>(bind_data.val_off);
					bind_data.selects = static_cast<WidgetCtxSelectSingle *>(ctx)->m_vars;
				}

				else if (ctx->m_type == WidgetType::SLIDER_INT) {
					*static_cast<int *>(var) = std::any_cast<int>(bind_data.val_off);
					bind_data.fmt = static_cast<WidgetCtxSliderInt *>(ctx)->m_fmt;
				}

				else if (ctx->m_type == WidgetType::SLIDER_FLOAT) {
					*static_cast<float *>(var) = std::any_cast<float>(bind_data.val_off);
					bind_data.fmt = static_cast<WidgetCtxSliderFloat *>(ctx)->m_fmt;
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

	ImGui::PushID(var);

	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		ImGui::OpenPopup("widget_popup_context");
	}

	bool open_bind{};

	if (ImGui::BeginPopup("widget_popup_context"))
	{
		ImGui::TextUnformatted(cfg_var->name().c_str());

		ImGui::Separator();

		if (ImGui::MenuItem(std::format("reset to default ({})", formatValue(cfg_var, cfg_var->defaultValue(), ctx)).c_str(), nullptr, false, !cfg_var->isDefault())) {
			cfg_var->reset();
		}

		if (ctx && ImGui::MenuItem("bind")) {
			open_bind = true;
		}

		ImGui::EndPopup();
	}

	if (ctx) {
		bindPopup(var, cfg_var, ctx, open_bind);
	}

	ImGui::PopID();
}

//==================================================================================================================================

bool toggle(const char *const label, bool *const var)
{
	const bool callback{ ImGui::Checkbox(label, var) };

	WidgetCtx ctx{ std::make_any<bool>(false), std::make_any<bool>(true) };
	widgetContext(label, var, &ctx);

	return callback;
}

bool selectSingle(const char *const label, int *const var, std::vector<std::pair<std::string, int>> vars)
{
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
	for (const std::pair<std::string, bool *> &var : vars) {
		registerWidget(std::format("{}: {}", label, var.first).c_str(), findCfgVar(var.second));
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

bool sliderInt(const char *const label, int *const var, int min, int max, const char *const fmt)
{
	const bool callback{ ImGui::SliderInt(label, var, min, max, fmt, ImGuiSliderFlags_AlwaysClamp) };

	WidgetCtxSliderInt ctx{ min, max, fmt };
	widgetContext(label, var, &ctx);

	return callback;
}

bool sliderFloat(const char *const label, float *const var, float min, float max, const char *const fmt)
{
	const bool callback{ ImGui::SliderFloat(label, var, min, max, fmt, ImGuiSliderFlags_AlwaysClamp) };

	WidgetCtxSliderFloat ctx{ min, max, fmt };
	widgetContext(label, var, &ctx);

	return callback;
}

bool colorPicker(const char *const label, Color *const var)
{
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
	const bool callback{ ImGui::InputText(label, var->data(), var->capacity() + 1, ImGuiInputTextFlags_CallbackResize | flags, resizeCallback, var) };

	widgetContext(label, var, nullptr);

	return callback;
}

bool inputStringWithHint(const char *const label, std::string *const var, const char *const hint, const char *const fmt, int flags)
{
	return ImGui::InputTextWithHint(label, hint, var->data(), var->capacity() + 1, ImGuiInputTextFlags_CallbackResize | flags, resizeCallback, var);
}

//==================================================================================================================================

void beginPage(const char *const id, const float bottom_padding)
{
	cur_page = id;

	ImGui::BeginChild(id, { 0.0f, -bottom_padding });
}

void endPage()
{
	cur_page.clear();

	ImGui::EndChild();
}

void beginColumn(const char *const id)
{
	ImGui::BeginChild(id, { 0.0f, 0.0f }, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
}

void endColumn()
{
	ImGui::EndChild();
}

void nextColumn()
{
	ImGui::SameLine();
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

		const bool any_changed{ std::any_of(sec->widgets.begin(), sec->widgets.end(), [](const RegisteredWidget &w) { return !w.var->isDefault(); }) };

		if (ImGui::MenuItem("reset section to defaults", nullptr, false, any_changed))
		{
			confirm(std::format("reset all '{}' settings to their defaults?", sec->title), [sec]()
			{
				for (const RegisteredWidget &w : sec->widgets) {
					w.var->reset();
				}

				notifs->message(std::format("reset '{}' to defaults", sec->title));
			});
		}

		ImGui::EndPopup();
	}
}

bool section(const char *const id, const char *const title, const float width, const std::function<void()> &body, const float item_width)
{
	ImGuiChildFlags flags{ ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY };

	//width of 0 fills the available space instead of fitting the contents
	if (width > 0.0f) {
		flags |= ImGuiChildFlags_AutoResizeX;
	}

	RegisteredSection *const sec{ findOrAddSection(id, title) };

	ImGui::BeginChild(id, { width, 0.0f }, flags);
	{
		ImGui::PushItemWidth(item_width > 0.0f ? item_width : ImGui::GetContentRegionAvail().x);

		ImGui::TextUnformatted(title);
		sectionContext(sec);
		ImGui::Separator();

		cur_section = sec;
		body();
		cur_section = nullptr;

		ImGui::PopItemWidth();
	}
	ImGui::EndChild();

	return ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_Stationary | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
}

void sep()
{
	ImGui::Separator();
}

void heading(const char *const text)
{
	ImGui::TextUnformatted(text);
}
