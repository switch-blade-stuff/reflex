/*
 * Created by switchblade on 2023-03-19.
 */

#pragma once

#include <algorithm>

#include "type_info.hpp"
#include "database.hpp"

namespace reflex
{
	bad_argument_list::~bad_argument_list() noexcept = default;

	tpp::dense_set<type_info, detail::str_hash, detail::str_cmp> type_info::parents() const
	{
		tpp::dense_set<type_info, detail::str_hash, detail::str_cmp> result;
		fill_parents(result);
		return result;
	}
	void type_info::fill_parents(tpp::dense_set<type_info, detail::str_hash, detail::str_cmp> &result) const
	{
		/* Recursively add parent types to the set. */
		result.reserve(result.capacity() + m_data->base_list.size());
		for (auto [_, base]: m_data->base_list)
		{
			const auto parent = type_info{base.type, *m_db};
			parent.fill_parents(result);
			result.emplace(parent);
		}
	}

	tpp::dense_map<std::string_view, any, detail::str_hash, detail::str_cmp> type_info::enumerations() const
	{
		tpp::dense_map<std::string_view, any, detail::str_hash, detail::str_cmp> result;
		result.reserve(m_data->enum_list.size());
		for (const auto &[name, value]: m_data->enum_list)
			result.emplace(name, value.ref());
		return result;
	}

	bool type_info::has_enumeration(const any &value) const
	{
		return std::ranges::any_of(m_data->enum_list, [&](auto &entry) { return entry.second == value; });
	}
	bool type_info::has_enumeration(std::string_view name) const
	{
		return m_data->enum_list.contains(name);
	}

	any type_info::enumerate(std::string_view name) const
	{
		const auto pos = m_data->enum_list.find(name);
		return pos != m_data->enum_list.end() ? pos->second.ref() : any{};
	}

	bool type_info::has_facet_vtable(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;
		if (m_data->find_facet(name) != nullptr)
			return true;

		const auto pred = [&](auto e) { return type_info{e.second.type, *m_db}.has_facet_vtable(name); };
		return std::ranges::any_of(m_data->base_list, pred);
	}
	bool type_info::inherits_from(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;
		if (m_data->find_base(name) != nullptr)
			return true;

		const auto pred = [&](auto e) { return type_info{e.second.type, *m_db}.inherits_from(name); };
		return std::ranges::any_of(m_data->base_list, pred);
	}
	bool type_info::convertible_to(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;
		if (m_data->find_conv(name) != nullptr)
			return true;

		const auto pred = [&](auto e) { return type_info{e.second.type, *m_db}.convertible_to(name); };
		return std::ranges::any_of(m_data->base_list, pred);
	}

	bool type_info::constructible_from(std::span<any> args) const
	{
		if (!valid()) [[unlikely]] return false;
		return m_data->find_ctor(args) != nullptr;
	}
	bool type_info::constructible_from(std::span<const detail::arg_data> args) const
	{
		if (!valid()) [[unlikely]] return false;
		return m_data->find_ctor(args) != nullptr;
	}

	any type_info::construct(std::span<any> args) const
	{
		if (valid()) [[likely]]
		{
			const auto *ctor = m_data->find_ctor(args);
			if (ctor != nullptr) return ctor->func(args);
		}
		return {};
	}

	bool type_info::comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;
		return m_data->find_cmp(name);
	}
	bool type_info::eq_comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto cmp = m_data->find_cmp(name);
		return cmp && cmp->cmp_eq && cmp->cmp_ne;
	}
	bool type_info::ge_comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto cmp = m_data->find_cmp(name);
		return cmp && cmp->cmp_ge;
	}
	bool type_info::le_comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto cmp = m_data->find_cmp(name);
		return cmp && cmp->cmp_le;
	}
	bool type_info::gt_comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto cmp = m_data->find_cmp(name);
		return cmp && cmp->cmp_gt;
	}
	bool type_info::lt_comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto cmp = m_data->find_cmp(name);
		return cmp && cmp->cmp_lt;
	}
}