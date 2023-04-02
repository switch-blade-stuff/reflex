/*
 * Created by switchblade on 2023-03-14.
 */

#pragma once

#include <cassert>

#include "database.hpp"

namespace reflex::detail
{
	database_impl *database_impl::instance() noexcept { return global_ptr(); }
	database_impl *database_impl::instance(database_impl *ptr) noexcept { return global_ptr().exchange(ptr); }

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
	void database_impl::reset(type_data &data)
	{
		const auto l = detail::scoped_lock{data};
		data.reset(*this);
	}
	void database_impl::reset(std::string_view name)
	{
		const auto l = detail::scoped_lock{*this};
		if (auto iter = m_types.find(name); iter != m_types.end())
			[[likely]] reset(iter->second);
	}

	const type_data *database_impl::find(std::string_view name) const
	{
		const auto l = detail::shared_scoped_lock{*this};
		if (const auto iter = m_types.find(name); iter != m_types.end())
			return &iter->second;
		else
			return nullptr;
	}

	type_data *database_impl::insert(const constant_type_data &data)
	{
		const auto l = detail::scoped_lock{*this};
		return &m_types.emplace(data.name, data).first->second.init(*this);
	}
}