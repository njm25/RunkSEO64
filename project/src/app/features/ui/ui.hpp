#pragma once

#include "../../../game/game.hpp"

#include <optional>

//everything the font atlas is built from, a change means a rebuild
struct FontSpec final
{
	std::string font{};
	int size{};
	int smoothing{};
	bool bold{};
	bool italic{};
	float letter_spacing{};
	float row_spacing{};
	float scale{};
	std::string preview{}; //extra font loaded for previewing in the font picker

	bool operator==(const FontSpec &) const = default;
};

struct FontPreset final
{
	std::string name{};
	std::string font{};
	int size{};
	int smoothing{};
	bool bold{};
	bool italic{};
	float letter_spacing{};
	float row_spacing{};
	bool preset{};
};

//everything a theme sets
struct ThemeValues final
{
	Color base{};
	bool accent_custom{};
	Color accent{};

	float rounding{};
	float border{ 1.0f };
	float padding{ 4.0f };

	int bg_style{}; //0 solid, 1 gradient, 2 image
	Color gradient{ 10, 10, 30, 255 };
	int gradient_dir{};
	std::string image{};
	float image_opacity{ 0.3f };
	int image_fit{};

	bool operator==(const ThemeValues &other) const;
};

struct Theme final
{
	std::string name{};
	ThemeValues values{};
	bool preset{};
};

class UI final : public HasLoad, public HasUnload
{
private:
	bool m_open{};

private:
	std::vector<Theme> m_themes{};
	std::optional<ThemeValues> m_preview_theme{};
	ThemeValues m_frame_theme{}; //what this frame is being drawn with, preview included

private:
	IDirect3DTexture9 *m_bg_texture{};
	std::string m_bg_texture_path{};
	ImVec2 m_bg_texture_size{};
	std::string m_bg_failed_path{}; //so a bad image is reported once instead of every frame

private:
	IDirect3DTexture9 *backgroundTexture(const std::string &path);

private:
	ImGuiStyle m_base_style{}; //unscaled
	float m_scale{};

private:
	FontSpec m_font_spec{}; //what the atlas was last built from
	ImFont *m_preview_font{};
	std::string m_preview_font_file{}; //last font asked for a preview, stays loaded until another is asked for
	std::vector<FontPreset> m_font_presets{};

private:
	FontSpec wantedFontSpec() const;
	void applyFonts(const FontSpec &spec);
	void loadFontPresets();
	void saveFontPresets() const;

private:
	void loadThemes();
	void saveThemes() const;

public:
	bool isOpen() const { return m_open; }
	float scale() const { return m_scale; }

public:
	const std::vector<Theme> &getThemes() const { return m_themes; }
	const Theme *findTheme(std::string_view name) const;
	std::string themeName(const ThemeValues &values) const; //"custom" when no theme matches
	void saveTheme(const std::string &name, const ThemeValues &values);
	void deleteTheme(const std::string &name);

	ThemeValues currentTheme() const; //from the theme settings
	void applyTheme(const ThemeValues &values); //to the theme settings

	//draws the next frame with this theme instead of the configured one
	void previewTheme(const ThemeValues &values) { m_preview_theme = values; }

	//gradient or image backgrounds replace the window background, call right after Begin
	bool customBackground() const { return m_frame_theme.bg_style != 0; }
	void drawBackground(ImDrawList *const draw_list, const ImVec2 &pos, const ImVec2 &size);

	//textures made here die with the device, called from the device reset hook
	void releaseDeviceObjects();

public:
	//loads a font file next to the menu font so it can be shown in the font picker, nullptr until it's ready
	void previewFont(std::string_view font) { m_preview_font_file = font; }
	ImFont *getPreviewFont(std::string_view font) const { return m_font_spec.preview == font ? m_preview_font : nullptr; }

	const std::vector<FontPreset> &getFontPresets() const { return m_font_presets; }
	const FontPreset *findFontPreset(std::string_view name) const;
	std::string currentFontPresetName() const; //"custom" when the font settings don't match a preset
	void applyFontPreset(const FontPreset &preset);
	void saveFontPreset(const std::string &name);
	void deleteFontPreset(const std::string &name);

public:
	bool onLoad() override;
	bool onUnload() override;

private:
	void setColors(const ThemeValues &theme);
	void setShape(const ThemeValues &theme);

public:
	void start();
	void end();
};

MAKE_UNIQUE(UI, ui);

//hardcoded menu sizes go through this so they follow the ui scale
inline float scaled(const float size)
{
	return size * ui->scale();
}

//pulls a saved window position back so at least a strip of the window stays on screen, e.g. after a resolution change
inline void keepOnScreen(float &x, float &y, const ImVec2 &size)
{
	const ImVec2 display{ ImGui::GetIO().DisplaySize };
	const float margin{ scaled(40.0f) };

	if (display.x <= 0.0f || display.y <= 0.0f) {
		return;
	}

	x = std::clamp(x, margin - size.x, std::max(display.x - margin, 0.0f));
	y = std::clamp(y, 0.0f, std::max(display.y - margin, 0.0f));
}
