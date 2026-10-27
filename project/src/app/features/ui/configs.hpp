#pragma once

#include "../../../game/game.hpp"

class Configs final : public HasLoad
{
private:
	std::string m_active{}; //last config loaded or saved
	nlohmann::json m_snapshot{}; //settings as of that load or save, for spotting unsaved changes

	mutable bool m_dirty{};
	mutable double m_dirty_check_time{ -1.0 };

private:
	void setActive(const std::string &name);

public:
	bool onLoad() override;

public:
	std::string dir() const;
	std::string pathOf(std::string_view name) const;
	std::vector<std::string> list() const;
	bool exists(std::string_view name) const;

	//empty when the name can be used as a file name, otherwise why it can't
	static std::string validateName(std::string_view name);

public:
	bool load(const std::string &name);
	bool save(const std::string &name);
	bool remove(const std::string &name);

	bool copyToClipboard(const std::string &name) const;
	bool importFromClipboard(const std::string &name);

public:
	const std::string &active() const { return m_active; }
	bool hasUnsavedChanges() const;
};

MAKE_UNIQUE(Configs, configs);
