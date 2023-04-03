/*
 * Created by switchblade on 2023-03-19.
 */

#pragma once

#include <algorithm>

#include "database.hpp"

namespace reflex
{
	bool constructor_info::is_invocable(argument_view args) const
	{
		return detail::arg_data::match_compatible(m_data->args, args, *m_db);
	}
	bool constructor_info::is_invocable(std::span<any> args) const
	{
		return detail::arg_data::match_compatible(m_data->args, args, *m_db);
	}

	detail::attr_map type_info::attributes() const
	{
		if (!valid()) [[unlikely]] return {};

		auto result = detail::attr_map{m_data->attrs.size()};
		for (const auto &[_, value]: m_data->attrs)
			result.emplace(value.type(), value.ref());
		return result;
	}
	any type_info::attribute(std::string_view name) const
	{
		if (!valid()) [[unlikely]] return {};
		const auto pos = m_data->attrs.find(name);
		return pos != m_data->attrs.end() ? pos->second.ref() : any{};
	}
	bool type_info::has_attribute(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;
		return m_data->find_attr(name);
	}

	detail::enum_map type_info::enumerations() const
	{
		if (!valid()) [[unlikely]] return {};

		auto result = detail::enum_map{m_data->enums.size()};
		for (const auto &[name, value]: m_data->enums)
			result.emplace(name, value.ref());
		return result;
	}
	any type_info::enumerate(std::string_view name) const
	{
		if (!valid()) [[unlikely]] return {};

		const auto pos = m_data->enums.find(name);
		return pos != m_data->enums.end() ? pos->second.ref() : any{};
	}
	bool type_info::has_enumeration(const any &value) const
	{
		if (!valid()) [[unlikely]] return false;
		return m_data->find_enum(value);
	}
	bool type_info::has_enumeration(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;
		return m_data->find_enum(name);
	}

	detail::type_set type_info::parents() const
	{
		detail::type_set result;
		fill_parents(result);
		return result;
	}
	bool type_info::inherits_from(std::string_view name) const
	{
		if (!valid()) [[unlikely]] return false;
		return m_data->find_base(name, *m_db);
	}

	bool type_info::implements_facet(std::string_view name) const
	{
		if (!valid()) [[unlikely]] return false;
		return m_data->find_vtab(name, *m_db);
	}

	any type_info::construct(std::span<any> args) const
	{
		if (valid()) [[likely]]
		{
			const auto *ctor = m_data->find_ctor(args, *m_db);
			if (ctor) return (*ctor)(args);
		}
		return {};
	}
	bool type_info::constructible_from(argument_view args) const
	{
		if (!valid()) [[unlikely]] return false;
		return m_data->find_ctor(args, *m_db);
	}
	bool type_info::constructible_from(std::span<any> args) const
	{
		if (!valid()) [[unlikely]] return false;
		return m_data->find_ctor(args, *m_db);
	}

	bool type_info::convertible_to(std::string_view name) const
	{
		if (!valid()) [[unlikely]] return false;
		return m_data->find_conv(name, *m_db);
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

	void type_info::fill_parents(detail::type_set &result) const
	{
		if (valid()) [[likely]]
		{
			/* Recursively add parent types to the set. */
			result.reserve(result.capacity() + m_data->bases.size());
			for (auto [_, base]: m_data->bases)
			{
				const auto parent = type_info{base.type, *m_db};
				parent.fill_parents(result);
				result.emplace(parent);
			}
		}
	}
	const void *type_info::get_vtab(std::string_view name) const
	{
		if (!valid()) [[unlikely]] return nullptr;
		return m_data->find_vtab(name, *m_db);
	}
}