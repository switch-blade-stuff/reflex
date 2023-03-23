/*
 * Created by switchblade on 2023-03-19.
 */

#pragma once

#include <algorithm>

#include "type_info.hpp"
#include "database.hpp"

namespace reflex
{
	tpp::dense_set<type_info> type_info::parents() const
	{
		tpp::dense_set<type_info> result;
		fill_parents(result);
		return result;
	}
	void type_info::fill_parents(tpp::dense_set<type_info> &result) const
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

	bool type_info::inherits_from(std::string_view name) const noexcept
	{
		/* Breadth-first search of parents. */
		if (m_data->find_base(name) != nullptr)
			return true;

		const auto pred = [&](auto e) { return type_info{e.second.type, *m_db}.inherits_from(name); };
		return std::ranges::any_of(m_data->base_list, pred);
	}
	bool type_info::convertible_to(std::string_view name) const noexcept
	{
		return m_data->find_conv(name) != nullptr;
	}
}