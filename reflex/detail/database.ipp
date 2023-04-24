/*
 * Created by switchblade on 2023-03-14.
 */

#pragma once

#include <cassert>

#include "database.hpp"

namespace reflex::detail
{
	database_impl *database_impl::instance() noexcept
	{
		for (auto ptr = global_ptr().load();;)
		{
			/* If the global pointer is set, return the existing pointer. */
			if (ptr != nullptr)
				return ptr;

			/* If the global pointer is not set, initialize from the local instance. */
			if (const auto new_ptr = local_ptr(); global_ptr().compare_exchange_weak(ptr, new_ptr))
				return new_ptr;
		}
	}
	database_impl *database_impl::instance(database_impl *ptr) noexcept
	{
		return global_ptr().exchange(ptr);
	}

	database_impl::database_impl() = default;
	database_impl::database_impl(database_impl &&other)
	{
		const auto l = detail::scoped_lock{other};
		std::construct_at(&m_types, std::move(other.m_types));
	}
	database_impl::~database_impl() = default;

	void database_impl::reset()
	{
		const auto l = detail::scoped_lock{*this};
		for (auto &[_, entry]: m_types) reset(entry);
	}
	void database_impl::reset(type_data &data) { data.reset(*this); }
	void database_impl::reset(std::string_view name)
	{
		const auto l = detail::scoped_lock{*this};
		if (auto iter = m_types.find(name); iter != m_types.end())
			[[likely]] reset(iter->second);
	}

	type_data *database_impl::find(std::string_view name)
	{
		const auto l = detail::shared_scoped_lock{*this};
		if (const auto iter = m_types.find(name); iter != m_types.end())
			return &iter->second;
		else
			return nullptr;
	}
	type_data *database_impl::insert(std::string_view name, const constant_type_data &data)
	{
		const auto l = detail::scoped_lock{*this};

		/* Type name must be copied into the database, and the entry must be post-initialized. */
		auto &[name_str, entry] = *m_types.emplace(name, data).first;
		entry.name = name_str;
		entry.init(*this);
		return &entry;
	}
}