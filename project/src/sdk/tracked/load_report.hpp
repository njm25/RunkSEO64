#pragma once

#include "../../shared/shared.hpp"

#include <memory>
#include <mutex>

enum class LoadKind
{
	SIG, IFACE, CVAR, HOOK, NETVAR, NETVAR_HOOK
};

struct LoadItem final
{
	LoadKind kind{};
	std::string name{};
	bool ok{};
	uintptr_t addr{};
	double ms{}; //time taken to resolve
	std::string detail{}; //why it failed, or anything else worth knowing
};

//everything the loader and the netvar lookups resolved, for the diagnostics tab
class LoadReport final
{
private:
	mutable std::mutex m_mutex{};
	std::vector<LoadItem> m_items{};

public:
	void add(LoadItem item)
	{
		const std::scoped_lock lock{ m_mutex };

		//netvars are looked up again by every NETVAR use, one entry each is enough
		if (item.kind == LoadKind::NETVAR)
		{
			for (const LoadItem &existing : m_items)
			{
				if (existing.kind == LoadKind::NETVAR && existing.name == item.name) {
					return;
				}
			}
		}

		m_items.push_back(std::move(item));
	}

	std::vector<LoadItem> items() const
	{
		const std::scoped_lock lock{ m_mutex };

		return m_items;
	}

	bool failed(const LoadKind kind, std::string_view name) const
	{
		const std::scoped_lock lock{ m_mutex };

		for (const LoadItem &item : m_items)
		{
			if (item.kind == kind && item.name == name) {
				return !item.ok;
			}
		}

		return false;
	}
};

inline const std::unique_ptr<LoadReport> load_report{ std::make_unique<LoadReport>() };
