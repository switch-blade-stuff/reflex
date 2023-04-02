/*
 * Created by switchblade on 2023-03-19.
 */

#pragma once

#include <algorithm>

#include "type_info.hpp"
#include "database.hpp"

namespace reflex
{
	tpp::dense_set<type_info, detail::type_hash, detail::type_eq> type_info::parents() const
	{
		if (!valid()) [[unlikely]] return {};

		const auto l = detail::shared_scoped_lock{*m_data};
		tpp::dense_set<type_info, detail::type_hash, detail::type_eq> result;
		fill_parents(result);
		return result;
	}
	void type_info::fill_parents(tpp::dense_set<type_info, detail::type_hash, detail::type_eq> &result) const
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
	bool type_info::inherits_from(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		if (m_data->find_base(name) != nullptr)
			return true;

		const auto pred = [&](auto e) { return type_info{e.second.type, *m_db}.inherits_from(name); };
		return std::ranges::any_of(m_data->bases, pred);
	}

	tpp::dense_map<std::string_view, any> type_info::enumerations() const
	{
		if (!valid()) [[unlikely]] return {};

		const auto l = detail::shared_scoped_lock{*m_data};
		tpp::dense_map<std::string_view, any> result;
		result.reserve(m_data->enums.size());
		for (const auto &[name, value]: m_data->enums)
			result.emplace(name, value.ref());
		return result;
	}
	bool type_info::has_enumeration(const any &value) const
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		return std::ranges::any_of(m_data->enums, [&](auto &&e) { return e.second == value; });
	}
	bool type_info::has_enumeration(std::string_view name) const
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		return m_data->enums.contains(name);
	}
	any type_info::enumerate(std::string_view name) const
	{
		if (!valid()) [[unlikely]] return {};

		const auto l = detail::shared_scoped_lock{*m_data};
		const auto pos = m_data->enums.find(name);
		return pos != m_data->enums.end() ? pos->second.ref() : any{};
	}

	bool type_info::has_facet_vtable(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		if (m_data->find_facet(name) != nullptr)
			return true;

		const auto pred = [&](auto e) { return type_info{e.second.type, *m_db}.has_facet_vtable(name); };
		return std::ranges::any_of(m_data->bases, pred);
	}

	bool type_info::convertible_to(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		if (m_data->find_conv(name) != nullptr)
			return true;

		const auto pred = [&](auto e) { return type_info{e.second.type, *m_db}.convertible_to(name); };
		return std::ranges::any_of(m_data->bases, pred);
	}

	bool type_info::constructible_from(std::span<any> args) const
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		return m_data->find_ctor(args) != nullptr;
	}
	bool type_info::constructible_from(std::span<const detail::arg_data> args) const
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		return m_data->find_ctor(args) != nullptr;
	}
	any type_info::construct(std::span<any> args) const
	{
		if (valid()) [[likely]]
		{
			const auto l = detail::shared_scoped_lock{*m_data};
			const auto *ctor = m_data->find_ctor(args);
			if (ctor != nullptr) return ctor->func(args);
		}
		return {};
	}

	bool type_info::comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		return m_data->find_cmp(name);
	}
	bool type_info::eq_comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		const auto cmp = m_data->find_cmp(name);
		return cmp && cmp->cmp_eq && cmp->cmp_ne;
	}
	bool type_info::ge_comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		const auto cmp = m_data->find_cmp(name);
		return cmp && cmp->cmp_ge;
	}
	bool type_info::le_comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		const auto cmp = m_data->find_cmp(name);
		return cmp && cmp->cmp_le;
	}
	bool type_info::gt_comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		const auto cmp = m_data->find_cmp(name);
		return cmp && cmp->cmp_gt;
	}
	bool type_info::lt_comparable_with(std::string_view name) const noexcept
	{
		if (!valid()) [[unlikely]] return false;

		const auto l = detail::shared_scoped_lock{*m_data};
		const auto cmp = m_data->find_cmp(name);
		return cmp && cmp->cmp_lt;
	}
}