#pragma once

#include <string>
#include <string_view>
#include <vector>

struct FontFile final
{
	std::string name{}; //full name from the font, e.g. "Segoe UI Bold"
	std::string file{}; //file name for fonts in a fonts folder, full path otherwise
};

//fonts in the windows and per user fonts folders that the menu can load (ttf, otf, ttc), sorted by name
const std::vector<FontFile> &installedFonts(const bool rescan = false);

//file name or path -> full path, empty when it can't be found
std::string resolveFontPath(std::string_view font);

//the font's full name from its name table, empty if the file isn't a font
std::string readFontName(const std::string &path);

//installed font's display name, or the file name when it isn't one
std::string fontDisplayName(std::string_view font);
