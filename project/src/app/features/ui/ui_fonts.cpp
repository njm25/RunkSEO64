#include "ui_fonts.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>

static std::string envVar(const char *const name)
{
#ifdef _WIN32
	char *val{};
	size_t len{};

	if (_dupenv_s(&val, &len, name) || !val) {
		return {};
	}

	const std::string str{ val };
	free(val);

	return str;
#else
	const char *const val{ std::getenv(name) };
	return val ? val : "";
#endif
}

static std::vector<std::filesystem::path> fontDirs()
{
	std::string windir{ envVar("WINDIR") };

	if (windir.empty()) {
		windir = "C:\\Windows";
	}

	std::vector<std::filesystem::path> dirs{ std::filesystem::path{ windir } / "Fonts" };

	//fonts installed for just the current user live here instead
	if (const std::string local{ envVar("LOCALAPPDATA") }; !local.empty()) {
		dirs.push_back(std::filesystem::path{ local } / "Microsoft" / "Windows" / "Fonts");
	}

	return dirs;
}

static bool isFontFile(const std::filesystem::path &path)
{
	std::string ext{ path.extension().string() };
	std::transform(ext.begin(), ext.end(), ext.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });

	return ext == ".ttf" || ext == ".otf" || ext == ".ttc";
}

std::string resolveFontPath(std::string_view font)
{
	if (font.empty()) {
		return {};
	}

	std::error_code ec{};

	//a path to a font somewhere else
	if (font.find_first_of("\\/") != std::string_view::npos) {
		return std::filesystem::is_regular_file(std::filesystem::path{ font }, ec) ? std::string{ font } : std::string{};
	}

	for (const std::filesystem::path &dir : fontDirs())
	{
		const std::filesystem::path path{ dir / std::string{ font } };

		if (std::filesystem::is_regular_file(path, ec)) {
			return path.string();
		}
	}

	return {};
}

//--------------------------------------------------------------------------------------------------
// truetype / opentype name table, just enough to get a font's full name

static uint16_t be16(const unsigned char *const p) { return static_cast<uint16_t>(p[0] << 8 | p[1]); }
static uint32_t be32(const unsigned char *const p) { return static_cast<uint32_t>(p[0]) << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

static bool readAt(std::ifstream &file, const uint32_t offset, unsigned char *const out, const size_t size)
{
	file.seekg(offset);
	file.read(reinterpret_cast<char *>(out), static_cast<std::streamsize>(size));

	return static_cast<size_t>(file.gcount()) == size;
}

static std::string utf16beToUtf8(const unsigned char *const data, const size_t size)
{
	std::string out{};

	for (size_t i{}; i + 1 < size; i += 2)
	{
		const uint16_t c{ be16(data + i) };

		if (c < 0x80) {
			out += static_cast<char>(c);
		}

		else if (c < 0x800) {
			out += static_cast<char>(0xC0 | (c >> 6));
			out += static_cast<char>(0x80 | (c & 0x3F));
		}

		//surrogate halves are skipped, font names outside the bmp aren't worth the trouble
		else if (c < 0xD800 || c > 0xDFFF) {
			out += static_cast<char>(0xE0 | (c >> 12));
			out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (c & 0x3F));
		}
	}

	return out;
}

std::string readFontName(const std::string &path)
{
	std::ifstream file{ path, std::ios::binary };

	if (!file.is_open()) {
		return {};
	}

	unsigned char header[12]{};

	if (!readAt(file, 0, header, sizeof(header))) {
		return {};
	}

	uint32_t font_offset{};

	//collections hold several fonts, the first one names the file well enough
	if (be32(header) == 0x74746366) // 'ttcf'
	{
		unsigned char first[4]{};

		if (be32(header + 8) == 0 || !readAt(file, 12, first, sizeof(first))) {
			return {};
		}

		font_offset = be32(first);

		if (!readAt(file, font_offset, header, sizeof(header))) {
			return {};
		}
	}

	const uint32_t version{ be32(header) };

	if (version != 0x00010000 && version != 0x4F54544F && version != 0x74727565) { // 1.0, 'OTTO', 'true'
		return {};
	}

	const uint16_t num_tables{ be16(header + 4) };

	uint32_t name_offset{};

	for (uint16_t t{}; t < num_tables && t < 256; t++)
	{
		unsigned char record[16]{};

		if (!readAt(file, font_offset + 12 + t * 16u, record, sizeof(record))) {
			return {};
		}

		if (be32(record) == 0x6E616D65) { // 'name'
			name_offset = be32(record + 8);
			break;
		}
	}

	if (!name_offset) {
		return {};
	}

	unsigned char name_header[6]{};

	if (!readAt(file, name_offset, name_header, sizeof(name_header))) {
		return {};
	}

	const uint16_t count{ be16(name_header + 2) };
	const uint32_t strings{ name_offset + be16(name_header + 4) };

	//best match wins: windows english full name, then any windows full name, then family names, then mac names
	std::string best{};
	int best_score{ -1 };

	for (uint16_t i{}; i < count && i < 1024; i++)
	{
		unsigned char record[12]{};

		if (!readAt(file, name_offset + 6 + i * 12u, record, sizeof(record))) {
			break;
		}

		const uint16_t platform{ be16(record) };
		const uint16_t language{ be16(record + 4) };
		const uint16_t name_id{ be16(record + 6) };
		const uint16_t length{ be16(record + 8) };
		const uint16_t offset{ be16(record + 10) };

		if ((name_id != 4 && name_id != 1) || (platform != 3 && platform != 1) || !length) {
			continue;
		}

		const int score{ (name_id == 4 ? 4 : 0) + (platform == 3 ? 2 : 0) + (platform == 3 && language == 0x409 ? 1 : 0) };

		if (score <= best_score) {
			continue;
		}

		std::vector<unsigned char> data(length);

		if (!readAt(file, strings + offset, data.data(), data.size())) {
			continue;
		}

		best = platform == 3 ? utf16beToUtf8(data.data(), data.size()) : std::string{ data.begin(), data.end() };
		best_score = score;
	}

	return best;
}

//--------------------------------------------------------------------------------------------------

const std::vector<FontFile> &installedFonts(const bool rescan)
{
	static std::vector<FontFile> fonts{};
	static bool scanned{};

	if (scanned && !rescan) {
		return fonts;
	}

	scanned = true;
	fonts.clear();

	const std::vector<std::filesystem::path> dirs{ fontDirs() };

	for (size_t d{}; d < dirs.size(); d++)
	{
		std::error_code ec{};

		for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator{ dirs[d], ec })
		{
			if (!entry.is_regular_file(ec) || !isFontFile(entry.path())) {
				continue;
			}

			const std::string file{ entry.path().filename().string() };

			//a user font with the same file name as a system one would load the system one anyway
			if (std::any_of(fonts.begin(), fonts.end(), [&file](const FontFile &f) { return f.file == file; })) {
				continue;
			}

			std::string name{ readFontName(entry.path().string()) };

			if (name.empty()) {
				continue;
			}

			fonts.push_back(FontFile{ name, file });
		}
	}

	std::sort(fonts.begin(), fonts.end(), [](const FontFile &a, const FontFile &b) { return a.name < b.name; });

	return fonts;
}

std::string fontDisplayName(std::string_view font)
{
	for (const FontFile &f : installedFonts())
	{
		if (f.file.size() == font.size() && std::equal(f.file.begin(), f.file.end(), font.begin(), [](const char a, const char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); })) {
			return f.name;
		}
	}

	return std::filesystem::path{ font }.filename().string();
}
