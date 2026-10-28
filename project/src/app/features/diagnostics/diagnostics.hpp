#pragma once

#include "../../../game/game.hpp"
#include "../../../sdk/tracked/load_report.hpp"

struct ModuleStamp final
{
	std::string name{};
	uint32_t timestamp{}; //pe header link time, changes with every build valve ships
	uint32_t size{};
};

struct ModuleOffset final
{
	std::string module{};
	uintptr_t offset{};
};

//windows side, in diagnostics_win.cpp
namespace diag_platform
{
	std::vector<ModuleStamp> moduleStamps();
	ModuleOffset moduleOf(const uintptr_t addr);
}

class Diagnostics final
{
private:
	std::unordered_map<std::string, std::string> m_unavailable{}; //cfg var name -> why
	std::vector<ModuleStamp> m_modules{};
	std::vector<std::string> m_changed_modules{};
	bool m_open_requested{};
	size_t m_failed_at_load{};
	mutable std::unordered_map<uintptr_t, ModuleOffset> m_offsets{};

public:
	//called once everything is hooked
	void finishLoad();

	//which cfg vars can't work because something they need failed to load
	void computeUnavailable();

	//compares against the module stamps saved last time and saves the new ones, returns the changed modules
	std::vector<std::string> checkGameVersion(const std::vector<ModuleStamp> &now);

public:
	//empty when the setting works
	std::string unavailableReason(const ConfigVar *const var) const;
	const std::unordered_map<std::string, std::string> &unavailable() const;

	bool gameUpdated() const;
	const std::vector<std::string> &changedModules() const;
	const std::vector<ModuleStamp> &modules() const;

	//true once after loading into an updated game with failures, the menu opens the tab for it
	bool consumeOpenRequest();

	size_t failedCount() const;
	size_t failedAtLoad() const; //cached, cheap enough for every frame
	std::string formatAddress(const LoadItem &item) const;
	std::string report() const;
};

MAKE_UNIQUE(Diagnostics, diagnostics);

const char *loadKindName(const LoadKind kind);
