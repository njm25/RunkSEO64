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
	void loadThemes();
	void saveThemes() const;

public:
	bool isOpen() const { return m_open; }

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