#include "ui.hpp"

#include "ui_fonts.hpp"

#include "../notifs/notifs.hpp"
#include "../cfg.hpp"

static const Theme preset_themes[]
{
	{ "default", { Color{ 100, 100, 100, 255 } }, true },
	{ "midnight", { Color{ 60, 80, 140, 255 } }, true },
	{ "ocean", { Color{ 40, 110, 130, 255 } }, true },
	{ "forest", { Color{ 60, 120, 70, 255 } }, true },
	{ "amber", { Color{ 150, 110, 40, 255 } }, true },
	{ "crimson", { Color{ 140, 50, 60, 255 } }, true },
	{ "rose", { Color{ 150, 80, 110, 255 } }, true },
	{ "amethyst", { Color{ 110, 70, 140, 255 } }, true },
	{ "graphite red", { Color{ 75, 75, 80, 255 }, true, Color{ 230, 70, 80, 255 } }, true },
	{ "graphite blue", { Color{ 75, 75, 80, 255 }, true, Color{ 70, 150, 240, 255 } }, true },
};

static bool sameColor(const Color a, const Color b)
{
	return a.r == b.r && a.g == b.g && a.b == b.b;
}

static nlohmann::json colorToJson(const Color clr)
{
	return { clr.r, clr.g, clr.b, clr.a };
}

static Color colorFromJson(const nlohmann::json &j)
{
	return Color{ j[0].get<uint8_t>(), j[1].get<uint8_t>(), j[2].get<uint8_t>(), j[3].get<uint8_t>() };
}

bool ThemeValues::operator==(const ThemeValues &other) const
{
	//the accent only matters when it's in use
	return sameColor(base, other.base) && accent_custom == other.accent_custom && (!accent_custom || sameColor(accent, other.accent));
}

ThemeValues UI::currentTheme() const
{
	return ThemeValues{ cfg::ui_color_base, cfg::ui_accent_custom, cfg::ui_color_accent };
}

void UI::applyTheme(const ThemeValues &values)
{
	cfg::ui_color_base = values.base;
	cfg::ui_accent_custom = values.accent_custom;

	if (values.accent_custom) {
		cfg::ui_color_accent = values.accent;
	}
}

static const FontPreset preset_fonts[]
{
	{ "default", "tahoma.ttf", 13, 0, false, false, 0.0f, 0.0f, true },
	{ "clean", "segoeui.ttf", 14, 2, false, false, 0.0f, 0.0f, true },
	{ "compact", "verdana.ttf", 11, 0, false, false, 0.0f, -1.0f, true },
	{ "big", "segoeui.ttf", 17, 2, false, false, 0.0f, 1.0f, true },
	{ "mono", "consola.ttf", 13, 1, false, false, 0.0f, 0.0f, true },
};

void UI::loadFontPresets()
{
	m_font_presets.assign(std::begin(preset_fonts), std::end(preset_fonts));

	std::ifstream file{ utils::getFilePath() + "fonts.json" };

	if (!file.is_open()) {
		return;
	}

	try
	{
		nlohmann::json j{};

		file >> j;

		for (const nlohmann::json &p : j)
		{
			FontPreset preset
			{
				p["name"].get<std::string>(),
				p["font"].get<std::string>(),
				p["size"].get<int>(),
				p["smoothing"].get<int>(),
				p["bold"].get<bool>(),
				p["italic"].get<bool>(),
				p["letter_spacing"].get<float>(),
				p["row_spacing"].get<float>(),
				false
			};

			if (!findFontPreset(preset.name)) {
				m_font_presets.push_back(preset);
			}
		}
	}

	catch (const std::exception &e) {
		notifs->warning(std::format("failed to load font presets ({})", e.what()));
	}
}

void UI::saveFontPresets() const
{
	nlohmann::json j = nlohmann::json::array();

	for (const FontPreset &p : m_font_presets)
	{
		if (p.preset) {
			continue;
		}

		j.push_back({
			{ "name", p.name }, { "font", p.font }, { "size", p.size }, { "smoothing", p.smoothing },
			{ "bold", p.bold }, { "italic", p.italic }, { "letter_spacing", p.letter_spacing }, { "row_spacing", p.row_spacing }
		});
	}

	std::ofstream file{ utils::getFilePath() + "fonts.json" };

	file << std::setw(4) << j;
}

const FontPreset *UI::findFontPreset(std::string_view name) const
{
	for (const FontPreset &p : m_font_presets)
	{
		if (p.name == name) {
			return &p;
		}
	}

	return nullptr;
}

std::string UI::currentFontPresetName() const
{
	for (const FontPreset &p : m_font_presets)
	{
		const bool same_font{ p.font.size() == cfg::ui_font.size() && std::equal(p.font.begin(), p.font.end(), cfg::ui_font.begin(), [](const char a, const char b) {
			return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
		}) };

		if (same_font && p.size == cfg::ui_font_size && p.smoothing == cfg::ui_font_smoothing && p.bold == cfg::ui_font_bold && p.italic == cfg::ui_font_italic
			&& p.letter_spacing == cfg::ui_font_letter_spacing && p.row_spacing == cfg::ui_font_row_spacing) {
			return p.name;
		}
	}

	return "custom";
}

void UI::applyFontPreset(const FontPreset &preset)
{
	cfg::ui_font = preset.font;
	cfg::ui_font_size = preset.size;
	cfg::ui_font_smoothing = preset.smoothing;
	cfg::ui_font_bold = preset.bold;
	cfg::ui_font_italic = preset.italic;
	cfg::ui_font_letter_spacing = preset.letter_spacing;
	cfg::ui_font_row_spacing = preset.row_spacing;
}

void UI::saveFontPreset(const std::string &name)
{
	const FontPreset preset{ name, cfg::ui_font, cfg::ui_font_size, cfg::ui_font_smoothing, cfg::ui_font_bold, cfg::ui_font_italic, cfg::ui_font_letter_spacing, cfg::ui_font_row_spacing, false };

	for (FontPreset &p : m_font_presets)
	{
		if (p.name != name) {
			continue;
		}

		//built in presets stay as they are
		if (p.preset) {
			return;
		}

		p = preset;
		saveFontPresets();

		return;
	}

	m_font_presets.push_back(preset);
	saveFontPresets();
}

void UI::deleteFontPreset(const std::string &name)
{
	m_font_presets.erase(std::remove_if(m_font_presets.begin(), m_font_presets.end(), [&name](const FontPreset &p) { return !p.preset && p.name == name; }), m_font_presets.end());

	saveFontPresets();
}

void UI::loadThemes()
{
	m_themes.assign(std::begin(preset_themes), std::end(preset_themes));

	std::ifstream file{ utils::getFilePath() + "themes.json" };

	if (!file.is_open()) {
		return;
	}

	try
	{
		nlohmann::json j{};

		file >> j;

		for (const nlohmann::json &t : j)
		{
			const std::string name{ t["name"].get<std::string>() };

			if (findTheme(name)) {
				continue;
			}

			//themes saved before there was more than a base color only have "color"
			ThemeValues values{ colorFromJson(t["color"]) };

			values.accent_custom = t.value("accent_custom", false);

			if (t.contains("accent")) {
				values.accent = colorFromJson(t["accent"]);
			}

			m_themes.push_back(Theme{ name, values, false });
		}
	}

	catch (const std::exception &e) {
		notifs->warning(std::format("failed to load themes ({})", e.what()));
	}
}

void UI::saveThemes() const
{
	nlohmann::json j = nlohmann::json::array();

	for (const Theme &theme : m_themes)
	{
		if (theme.preset) {
			continue;
		}

		j.push_back({
			{ "name", theme.name },
			{ "color", colorToJson(theme.values.base) },
			{ "accent_custom", theme.values.accent_custom },
			{ "accent", colorToJson(theme.values.accent) },
		});
	}

	std::ofstream file{ utils::getFilePath() + "themes.json" };

	file << std::setw(4) << j;
}

const Theme *UI::findTheme(std::string_view name) const
{
	for (const Theme &theme : m_themes)
	{
		if (theme.name == name) {
			return &theme;
		}
	}

	return nullptr;
}

std::string UI::themeName(const ThemeValues &values) const
{
	for (const Theme &theme : m_themes)
	{
		if (theme.values == values) {
			return theme.name;
		}
	}

	return "custom";
}

void UI::saveTheme(const std::string &name, const ThemeValues &values)
{
	for (Theme &theme : m_themes)
	{
		if (theme.name != name) {
			continue;
		}

		//presets stay as they are
		if (theme.preset) {
			return;
		}

		theme.values = values;
		saveThemes();

		return;
	}

	m_themes.push_back(Theme{ name, values, false });
	saveThemes();
}

void UI::deleteTheme(const std::string &name)
{
	m_themes.erase(std::remove_if(m_themes.begin(), m_themes.end(), [&name](const Theme &theme) { return !theme.preset && theme.name == name; }), m_themes.end());

	saveThemes();
}

bool UI::onLoad()
{
	loadThemes();

	if (!ImGui::CreateContext()) {
		return false;
	}

	if (!ImGui_ImplWin32_Init(FindWindowA("Valve001", nullptr))) {
		return false;
	}

	if (!ImGui_ImplDX9_Init(i::dxdev)) {
		return false;
	}

	if (ImGuiIO *const io{ &ImGui::GetIO() })
	{
		io->IniFilename = nullptr;
		io->LogFilename = nullptr;

		io->MouseDrawCursor = false;

		io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	}

	if (ImGuiStyle *const style{ &ImGui::GetStyle() })
	{
		style->WindowPadding = { 4.0f, 4.0f };
		style->FramePadding = { 2.0f, 2.0f };
		style->ItemSpacing = { 4.0f, 4.0f };
		style->ItemInnerSpacing = { 4.0f, 4.0f };
		style->TouchExtraPadding = { 0.0f, 0.0f };

		style->IndentSpacing = 20.0f;
		style->ScrollbarSize = 10.0f;
		style->GrabMinSize = 6.0f;

		style->WindowBorderSize = 1.0f;
		style->ChildBorderSize = 1.0f;
		style->FrameBorderSize = 1.0f;
		style->PopupBorderSize = 1.0f;
		style->TabBorderSize = 1.0f;
		style->TabBarBorderSize = 1.0f;
		style->TabBarOverlineSize = 1.0f;

		style->WindowRounding = 0.0f;
		style->ChildRounding = 0.0f;
		style->FrameRounding = 0.0f;
		style->PopupRounding = 0.0f;
		style->ScrollbarRounding = 0.0f;
		style->GrabRounding = 0.0f;
		style->TabRounding = 0.0f;

		style->AntiAliasedFill = false;
		style->AntiAliasedLines = false;
		style->AntiAliasedLinesUseTex = false;

		m_base_style = *style;
	}

	loadFontPresets();
	applyFonts(wantedFontSpec());

	return true;
}

FontSpec UI::wantedFontSpec() const
{
	return FontSpec
	{
		cfg::ui_font,
		std::clamp(cfg::ui_font_size, 8, 32),
		std::clamp(cfg::ui_font_smoothing, 0, 2),
		cfg::ui_font_bold,
		cfg::ui_font_italic,
		std::clamp(cfg::ui_font_letter_spacing, -1.0f, 3.0f),
		std::clamp(cfg::ui_font_row_spacing, -2.0f, 4.0f),
		std::clamp(cfg::ui_scale, 0.75f, 2.0f),
		m_preview_font_file
	};
}

//rebuilds the font atlas and scales every style size, has to happen outside of a frame
void UI::applyFonts(const FontSpec &spec)
{
	ImGuiIO &io{ ImGui::GetIO() };

	//whole pixel sizes keep the sharp font crisp
	const float px{ std::floor(static_cast<float>(spec.size) * spec.scale + 0.5f) };

	auto addFont = [&](const std::string &path) -> ImFont *
	{
		//imgui asserts on files that aren't fonts, so anything without a readable name table is turned away first
		if (path.empty() || readFontName(path).empty()) {
			return nullptr;
		}

		ImFontConfig font_cfg{};
		{
			font_cfg.OversampleH = 1;
			font_cfg.OversampleV = 1;
			font_cfg.PixelSnapH = true;
			font_cfg.GlyphExtraSpacing.x = std::round(spec.letter_spacing * spec.scale);

			if (spec.smoothing == 0) {
				font_cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Monochrome | ImGuiFreeTypeBuilderFlags_MonoHinting;
			}

			else if (spec.smoothing == 2) {
				font_cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LightHinting;
			}

			if (spec.bold) {
				font_cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Bold;
			}

			if (spec.italic) {
				font_cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Oblique;
			}
		}

		return io.Fonts->AddFontFromFileTTF(path.c_str(), px, &font_cfg);
	};

	io.Fonts->Clear();
	m_preview_font = nullptr;

	//whichever font ends up as the menu font, empty for imgui's own
	std::string main_path{ resolveFontPath(spec.font) };

	auto addMain = [&]() -> bool
	{
		io.FontDefault = main_path.empty() ? io.Fonts->AddFontDefault() : addFont(main_path);

		return io.FontDefault && io.Fonts->Build();
	};

	//a missing or broken font falls back to tahoma, then to imgui's own font
	if (main_path.empty() || !addMain())
	{
		notifs->warning(std::format("couldn't load the font '{}', using tahoma", spec.font));

		io.Fonts->Clear();
		main_path = resolveFontPath("tahoma.ttf");

		if (!addMain()) {
			io.Fonts->Clear();
			main_path.clear();
			addMain();
		}
	}

	//the preview font goes in separately, so a broken one can't take the menu font down with it
	if (!spec.preview.empty())
	{
		m_preview_font = addFont(resolveFontPath(spec.preview));

		if (m_preview_font && !io.Fonts->Build())
		{
			m_preview_font = nullptr;

			//clearing drops every font, so the menu font has to be added again
			io.Fonts->Clear();
			addMain();
		}
	}

	//the backend recreates the font texture on its next NewFrame
	ImGui_ImplDX9_InvalidateDeviceObjects();

	ImGuiStyle &style{ ImGui::GetStyle() };

	style = m_base_style;
	style.ScaleAllSizes(spec.scale);
	style.ItemSpacing.y = std::max(0.0f, style.ItemSpacing.y + std::round(spec.row_spacing * spec.scale));

	m_scale = spec.scale;

	//even when loading failed, so a bad font isn't retried every frame
	m_font_spec = spec;
}

bool UI::onUnload()
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	return true;
}

void UI::setColors(const ThemeValues &theme)
{
	float hue{};
	float sat{};
	float val{};

	utils::rgbToHSV(theme.base, hue, sat, val);

	if (ImVec4 *const colors{ ImGui::GetStyle().Colors })
	{
		auto getColor = [&](float h, float s, float v)
		{
			h = math::remap(h, 0.0f, 360.0f, 0.0f, 1.0f);
			s = math::remap(s * sat, 0.0f, 100.0f, 0.0f, 1.0f);
			v = math::remap(v * val, 0.0f, 100.0f, 0.0f, 1.0f);

			if (s == 0.0f) {
				return ImVec4(v, v, v, 1.0f);
			}

			int i{ static_cast<int>(h * 6.0f) };

			float f{ (h * 6.0f) - i };
			float p{ v * (1.0f - s) };
			float q{ v * (1.0f - s * f) };
			float t{ v * (1.0f - s * (1.0f - f)) };

			i %= 6;

			if (i == 0) {
				return ImVec4(v, t, p, 1.0f);
			}

			else if (i == 1) {
				return ImVec4(q, v, p, 1.0f);
			}

			else if (i == 2) {
				return ImVec4(p, v, t, 1.0f);
			}

			else if (i == 3) {
				return ImVec4(p, q, v, 1.0f);
			}

			else if (i == 4) {
				return ImVec4(t, p, v, 1.0f);
			}

			else {
				return ImVec4(v, p, q, 1.0f);
			}
		};

		colors[ImGuiCol_Text] = ImVec4(0.7f, 0.7f, 0.7f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);

		colors[ImGuiCol_NavHighlight] = ImVec4(0.7f, 0.7f, 0.7f, 1.00f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.7f, 0.7f, 0.7f, 1.00f);

		colors[ImGuiCol_WindowBg] = getColor(hue, 0.75f, 0.08f);
		colors[ImGuiCol_PopupBg] = getColor(hue, 0.75f, 0.08f);
		colors[ImGuiCol_Border] = getColor(hue, 0.81f, 0.31f);
		colors[ImGuiCol_FrameBg] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_FrameBgHovered] = getColor(hue, 0.80f, 0.20f);
		colors[ImGuiCol_FrameBgActive] = getColor(hue, 0.80f, 0.20f);
		colors[ImGuiCol_TitleBg] = getColor(hue, 0.75f, 0.08f);
		colors[ImGuiCol_TitleBgActive] = getColor(hue, 0.75f, 0.08f);
		colors[ImGuiCol_TitleBgCollapsed] = getColor(hue, 0.75f, 0.08f);
		colors[ImGuiCol_CheckMark] = getColor(hue, 0.91f, 0.36f);
		colors[ImGuiCol_SliderGrab] = getColor(hue, 0.80f, 0.39f);
		colors[ImGuiCol_SliderGrabActive] = getColor(hue, 0.80f, 0.39f);
		colors[ImGuiCol_Button] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_ButtonHovered] = getColor(hue, 0.80f, 0.20f);
		colors[ImGuiCol_ButtonActive] = getColor(hue, 0.80f, 0.20f);
		colors[ImGuiCol_Header] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_HeaderHovered] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_HeaderActive] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_Separator] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_SeparatorHovered] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_SeparatorActive] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_ResizeGrip] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_ResizeGripHovered] = getColor(hue, 0.80f, 0.20f);
		colors[ImGuiCol_ResizeGripActive] = getColor(hue, 0.80f, 0.20f);
		colors[ImGuiCol_TabHovered] = getColor(hue, 0.75f, 0.16f);
		colors[ImGuiCol_Tab] = getColor(hue, 0.75f, 0.08f);
		colors[ImGuiCol_TabSelected] = getColor(hue, 0.80f, 0.20f);
		colors[ImGuiCol_TableHeaderBg] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_TableRowBg] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_TableRowBgAlt] = getColor(hue, 0.80f, 0.16f);
		colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_CheckMark];

		//a separate accent for everything that marks something as on, selected or focused
		if (theme.accent_custom)
		{
			const ImVec4 accent{ theme.accent.r / 255.0f, theme.accent.g / 255.0f, theme.accent.b / 255.0f, 1.0f };

			auto withAlpha = [&accent](const float a) {
				return ImVec4{ accent.x, accent.y, accent.z, a };
			};

			auto scaled = [&accent](const float f) {
				return ImVec4{ accent.x * f, accent.y * f, accent.z * f, 1.0f };
			};

			colors[ImGuiCol_CheckMark] = accent;
			colors[ImGuiCol_SliderGrab] = scaled(0.85f);
			colors[ImGuiCol_SliderGrabActive] = accent;
			colors[ImGuiCol_TabSelectedOverline] = accent;
			colors[ImGuiCol_Header] = withAlpha(0.35f);
			colors[ImGuiCol_HeaderActive] = withAlpha(0.5f);
			colors[ImGuiCol_TextSelectedBg] = withAlpha(0.35f);
			colors[ImGuiCol_NavHighlight] = accent;
			colors[ImGuiCol_DragDropTarget] = accent;
			colors[ImGuiCol_ResizeGripActive] = accent;
			colors[ImGuiCol_SeparatorActive] = accent;
		}
	}
}

void UI::start()
{
	if (input_dx->getKey(VK_INSERT).pressed) {
		m_open = !m_open;
		i::input_system->ResetInputState();
	}

	//waits for the mouse to be let go so dragging a slider doesn't rebuild the font every frame,
	//but not for a focused text field (the font picker's filter is focused while previews load)
	if (const FontSpec spec{ wantedFontSpec() }; spec != m_font_spec && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		applyFonts(spec);
	}

	const ThemeValues theme{ m_preview_theme.value_or(currentTheme()) };

	m_preview_theme.reset();

	setColors(theme);

	ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = std::clamp(cfg::ui_bg_opacity, 0.2f, 1.0f);

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();
}

void UI::end()
{
	DWORD color_write_enable{};
	DWORD srgb_write_enable{};
	DWORD color_op_stage_state{};
	DWORD alpha_op_stage_state{};

	i::dxdev->GetRenderState(D3DRS_COLORWRITEENABLE, &color_write_enable);
	i::dxdev->GetRenderState(D3DRS_SRGBWRITEENABLE, &srgb_write_enable);
	i::dxdev->GetTextureStageState(0, D3DTSS_COLOROP, &color_op_stage_state);
	i::dxdev->GetTextureStageState(0, D3DTSS_ALPHAOP, &alpha_op_stage_state);

	i::dxdev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xFFFFFFFF);
	i::dxdev->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
	i::dxdev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	i::dxdev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	i::dxdev->SetRenderState(D3DRS_COLORWRITEENABLE, color_write_enable);
	i::dxdev->SetRenderState(D3DRS_SRGBWRITEENABLE, srgb_write_enable);
	i::dxdev->SetTextureStageState(0, D3DTSS_COLOROP, color_op_stage_state);
	i::dxdev->SetTextureStageState(0, D3DTSS_ALPHAOP, alpha_op_stage_state);
}