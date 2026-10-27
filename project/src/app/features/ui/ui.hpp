#pragma once

#include "../../../game/game.hpp"

#include <optional>

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
	void applyScale(const float scale);

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
