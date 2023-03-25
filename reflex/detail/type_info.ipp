/*
 * Created by switchblade on 2023-03-19.
 */

#pragma once

#include <algorithm>

#include "type_info.hpp"
#include "database.hpp"

namespace reflex
{
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
	bool type_info::has_property(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;
		if (m_data->find_prop(name) != nullptr)
			return true;

		const auto pred = [&](auto e) { return type_info{e.second.type, *m_db}.has_property(name); };
		return std::ranges::any_of(m_data->base_list, pred);
	}
}