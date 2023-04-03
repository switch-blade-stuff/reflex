/*
 * Created by switchblade on 2023-04-01.
 */

#pragma once

#include <algorithm>

#include "query.hpp"

namespace reflex
{
	template<>
	detail::type_set type_query<>::types() const
	{
		detail::type_set result;
		for (auto &[_, type]: m_db->m_types)
			result.insert(type_info{&type, m_db});
		return result;
	}
	template<>
	detail::type_set type_query<>::filter(std::span<const filter_func> filters) const
	{
		if (filters.empty()) return types();
		detail::type_set result;

		/* Fill the set with initial elements. */
		for (auto &[_, type]: m_db->m_types)
		{
			if (filters[0](this, type_info{&type, m_db}))
				result.insert({&type, m_db});
		}

		/* Filter bad elements from the set. */
		for (std::size_t i = 1; !result.empty() && i < filters.size(); ++i)
		{
			/* Filter out remaining elements from the set. */
			for (auto pos = result.end(); --pos != result.begin();)
			{
				if (!filters[i](this, *pos))
					pos = result.erase(pos);
			}
		}

		return result;
	}
}