/*
 * Created by switchblade on 2023-03-19.
 */

#pragma once

#include <algorithm>

#include "type_info.hpp"
#include "database.hpp"

namespace reflex
{
	type_info::type_info(detail::type_handle handle, detail::database_impl &db) : m_data(handle(db)), m_db(&db) {}

	type_info type_info::get(std::string_view name)
	{
		auto *db = detail::database_impl::instance();
		return {db->find(name), db};
	}
	template<typename T>
	type_info type_info::get()
	{
		auto *db = detail::database_impl::instance();
		return {detail::make_type_data<std::remove_cvref_t<T>>, *db};
	}

	constexpr std::string_view type_info::name() const noexcept { return m_data->name; }
	constexpr std::size_t type_info::extent() const noexcept { return m_data->extent; }
	constexpr std::size_t type_info::size() const noexcept { return m_data->size; }

	constexpr bool type_info::is_empty() const noexcept { return size() == 0; }
	constexpr bool type_info::is_void() const noexcept { return m_data->flags & detail::IS_VOID; }
	constexpr bool type_info::is_nullptr() const noexcept { return m_data->flags & detail::IS_NULL; }

	constexpr bool type_info::is_array() const noexcept { return extent() > 0; }
	constexpr bool type_info::is_enum() const noexcept { return m_data->flags & detail::IS_ENUM; }
	constexpr bool type_info::is_class() const noexcept { return m_data->flags & detail::IS_CLASS; }

	constexpr bool type_info::is_pointer() const noexcept { return m_data->flags & detail::IS_POINTER; }
	constexpr bool type_info::is_integral() const noexcept { return m_data->flags & (detail::IS_SIGNED_INT | detail::IS_UNSIGNED_INT); }
	constexpr bool type_info::is_signed_integral() const noexcept { return m_data->flags & detail::IS_SIGNED_INT; }
	constexpr bool type_info::is_unsigned_integral() const noexcept { return m_data->flags & detail::IS_UNSIGNED_INT; }
	constexpr bool type_info::is_arithmetic() const noexcept { return m_data->flags & detail::IS_ARITHMETIC; }

	type_info type_info::remove_extent() const noexcept { return {m_data->remove_extent, *m_db}; }
	type_info type_info::remove_pointer() const noexcept { return {m_data->remove_pointer, *m_db}; }

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
		for (auto [_, base] : m_data->base_list)
		{
			const auto parent = type_info{base.type, *m_db};
			parent.fill_parents(result);
			result.emplace(parent);
		}
	}

	bool type_info::inherits_from(std::string_view name) const noexcept
	{
		/* Breadth-first search of parents. */
		if (find_base(name) != nullptr)
			return true;

		const auto iter = std::ranges::find_if(m_data->base_list, [&](auto e) { return type_info{e.second.type, *m_db}.inherits_from(name); });
		return iter != m_data->base_list.end();
	}
	const detail::type_base *type_info::find_base(std::string_view name) const
	{
		if (auto iter = m_data->base_list.find(name); iter != m_data->base_list.end())
			return &iter->second;
		else
			return nullptr;
	}
	const detail::type_conv *type_info::find_conv(std::string_view name) const
	{
		if (auto iter = m_data->conv_list.find(name); iter != m_data->conv_list.end())
			return &iter->second;
		else
			return nullptr;
	}
}