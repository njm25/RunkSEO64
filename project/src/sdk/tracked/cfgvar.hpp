#pragma once

#include "../../shared/shared.hpp"

#include <any>

class ConfigVar final : public InstTracker<ConfigVar>
{
private:
	std::string m_name{};
	void *m_ptr{};
	size_t m_type_hash{};
	std::any m_default{};

public:
	template <typename T>
	ConfigVar(std::string_view name, T *const ptr) : InstTracker(this)
	{
		m_name = name;
		m_ptr = ptr;
		m_type_hash = typeid(T).hash_code();
		m_default = *ptr; //cfg vars are initialized before their ConfigVar, so this is the default value
	}

public:
	std::string name() const
	{
		return m_name;
	}

	void *const ptr() const
	{
		return m_ptr;
	}

	size_t hash() const
	{
		return m_type_hash;
	}

	const std::any &defaultValue() const
	{
		return m_default;
	}

public:
	bool isDefault() const;
	void reset();
};

#define MAKE_CFGVAR(name, val) \
namespace cfg { inline auto name{ val }; } \
namespace make_cfgvar { inline ConfigVar name{ #name, &cfg::name }; }

namespace cfg
{
	nlohmann::json toJson();
	void fromJson(const nlohmann::json &j);

	void save(std::string_view name);
	bool load(std::string_view name);
}