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

struct Theme final
{
	std::string name{};
	Color color{};
	bool preset{};
};

class UI final : public HasLoad, public HasUnload
{
private:
	bool m_open{};

private:
	std::vector<Theme> m_themes{};
	std::optional<Color> m_preview_color{};

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
	std::string themeName(const Color clr) const; //"custom" when no theme uses the color
	void saveTheme(const std::string &name, const Color clr);
	void deleteTheme(const std::string &name);

	//draws the next frame with this base color instead of the configured one
	void previewColor(const Color clr) { m_preview_color = clr; }

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
	void setColors(const float hue, const float sat, const float val);

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
