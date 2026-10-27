#include "configs.hpp"

#include "../notifs/notifs.hpp"
#include "../cfg.hpp"

//window positions and sizes change just by using the menu, so they don't count as unsaved changes
static nlohmann::json withoutLayout(nlohmann::json j)
{
	for (auto it{ j.begin() }; it != j.end();)
	{
		const std::string &key{ it.key() };

		if (key.starts_with("ui_pos_") || key.starts_with("ui_menu_") || key == "spectator_list_x" || key == "spectator_list_y") {
			it = j.erase(it);
		}

		else {
			++it;
		}
	}

	return j;
}

void Configs::setActive(const std::string &name)
{
	m_active = name;
	m_snapshot = withoutLayout(cfg::toJson());
	m_dirty = false;
}

bool Configs::onLoad()
{
	//main loads default.json before anything else
	if (exists("default")) {
		setActive("default");
	}

	return true;
}

std::string Configs::dir() const
{
	const std::filesystem::path path{ std::filesystem::path{ utils::getFilePath() } / "cfg" };

	if (!std::filesystem::exists(path)) {
		std::filesystem::create_directories(path);
	}

	return path.string();
}

std::string Configs::pathOf(std::string_view name) const
{
	return (std::filesystem::path{ dir() } / std::format("{}.json", name)).string();
}

std::vector<std::string> Configs::list() const
{
	std::vector<std::string> names{};

	for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator{ dir() })
	{
		if (file.path().extension() == ".json") {
			names.push_back(file.path().stem().string());
		}
	}

	std::sort(names.begin(), names.end());

	return names;
}

bool Configs::exists(std::string_view name) const
{
	return std::filesystem::exists(pathOf(name));
}

std::string Configs::validateName(std::string_view name)
{
	if (name.empty()) {
		return "enter a name";
	}

	if (name.size() > 64) {
		return "name is too long";
	}

	for (const char c : name)
	{
		if (static_cast<unsigned char>(c) < 32 || std::string_view{ "<>:\"/\\|?*" }.find(c) != std::string_view::npos) {
			return "name can't contain < > : \" / \\ | ? *";
		}
	}

	if (name.back() == '.' || name.back() == ' ') {
		return "name can't end with a dot or space";
	}

	std::string upper{ name };
	std::transform(upper.begin(), upper.end(), upper.begin(), [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });

	static const std::unordered_set<std::string> reserved
	{
		"CON", "PRN", "AUX", "NUL",
		"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
		"LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
	};

	if (reserved.contains(upper)) {
		return std::format("'{}' is reserved by windows", name);
	}

	return {};
}

bool Configs::load(const std::string &name)
{
	if (!cfg::load(pathOf(name))) {
		notifs->warning(std::format("failed to load '{}'", name));
		return false;
	}

	setActive(name);
	notifs->message(std::format("loaded '{}'", name));

	return true;
}

bool Configs::save(const std::string &name)
{
	if (!validateName(name).empty()) {
		return false;
	}

	const bool existed{ exists(name) };

	cfg::save(pathOf(name));

	setActive(name);
	notifs->message(std::format("{} '{}'", existed ? "saved" : "created", name));

	return true;
}

bool Configs::remove(const std::string &name)
{
	std::error_code ec{};

	if (!std::filesystem::remove(pathOf(name), ec)) {
		notifs->warning(std::format("failed to delete '{}'", name));
		return false;
	}

	if (m_active == name) {
		m_active.clear();
	}

	notifs->message(std::format("deleted '{}'", name));

	return true;
}

bool Configs::copyToClipboard(const std::string &name) const
{
	std::ifstream file{ pathOf(name) };

	if (!file.is_open()) {
		return false;
	}

	const std::string contents{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };

	ImGui::SetClipboardText(contents.c_str());
	notifs->message(std::format("copied '{}' to clipboard", name));

	return true;
}

bool Configs::importFromClipboard(const std::string &name)
{
	if (!validateName(name).empty()) {
		return false;
	}

	const char *const clipboard{ ImGui::GetClipboardText() };

	nlohmann::json j{};

	try {
		j = nlohmann::json::parse(clipboard ? clipboard : "");
	}

	catch (const std::exception &) {
		notifs->warning("clipboard doesn't contain a config");
		return false;
	}

	//anything that doesn't share at least one setting with us isn't a config
	const bool any_known{ j.is_object() && std::any_of(getInsts<ConfigVar>().begin(), getInsts<ConfigVar>().end(), [&j](const ConfigVar *const var) {
		return j.contains(var->name());
	}) };

	if (!any_known) {
		notifs->warning("clipboard doesn't contain a config");
		return false;
	}

	std::ofstream file{ pathOf(name) };

	if (!file.is_open()) {
		notifs->warning(std::format("failed to write '{}'", name));
		return false;
	}

	file << std::setw(4) << j;

	notifs->message(std::format("imported '{}'", name));

	return true;
}

bool Configs::hasUnsavedChanges() const
{
	if (m_active.empty()) {
		return false;
	}

	//building the json for every var isn't free, a few times a second is plenty
	const double now{ ImGui::GetTime() };

	if (m_dirty_check_time < 0.0 || now - m_dirty_check_time > 0.25 || now < m_dirty_check_time) {
		m_dirty = withoutLayout(cfg::toJson()) != m_snapshot;
		m_dirty_check_time = now;
	}

	return m_dirty;
}
