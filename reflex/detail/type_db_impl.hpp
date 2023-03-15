/*
 * Created by switchblade on 2023-03-14.
 */

#pragma once

#include "type_db.hpp"

namespace reflex
{
	shared_guard<type_database *> type_database::instance() { return shared_guarded_instance<type_database>::instance(); }
	type_database *type_database::instance(type_database *ptr) { return shared_guarded_instance<type_database>::instance(ptr); }

	const detail::type_data *type_database::data_or(std::string_view name, const detail::type_data &init) const
	{
		const auto iter = m_types.find(name);
		return iter == m_types.end() ? &init : &iter->second;
	}
}