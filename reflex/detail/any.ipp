/*
 * Created by switchblade on 2023-03-22.
 */

#include "database.hpp"
#include "facets/compare.hpp"

namespace reflex
{
	any any::try_cast(type_info type)
	{
		/* If `this` is empty, or `type` is invalid, return empty any. */
		if (empty() || !type.valid()) return {};

		/* If `type` is the same as or is a base of `this`, return reference to `this`. */
		if (type == this->type()) return ref();
		/* If `this` is convertible to `type`, convert `this` to `type`. */
		if (const auto *base_ptr = base_cast(type.name()); base_ptr != nullptr)
		{
			if (!is_const())
				return any{type, const_cast<void *>(base_ptr)};
			else
				return any{type, base_ptr};
		}
		/* Otherwise, attempt to convert by-value. */
		return value_conv(type.name());
	}
	any any::try_cast(type_info type) const
	{
		/* If `this` is empty, or `type` is invalid, return empty any. */
		if (empty() || !type.valid()) return {};

		/* If `type` is the same as or is a base of `this`, return reference to `this`. */
		if (type == this->type()) return ref();
		/* If `this` is convertible to `type`, convert `this` to `type`. */
		if (const auto *base_ptr = base_cast(type.name()); base_ptr != nullptr)
			return any{type, base_ptr};
		/* Otherwise, attempt to convert by-value. */
		return value_conv(type.name());
	}

	void *any::base_cast(std::string_view base_name) const
	{
		const auto data = type_data();

		/* If `type` is an immediate base of `this`, attempt to cast through that. */
		if (const auto *base = data->find_base(base_name, *m_db); base != nullptr)
			return const_cast<void *>(base->cast_func(cdata()));

		/* Otherwise, recursively cast through a base type. */
		for (auto [_, base]: data->bases)
		{
			const auto *base_ptr = base.cast_func(cdata());
			const auto base_type = type_info{base.type, *m_db};
			auto candidate = any{base_type, base_ptr}.base_cast(base_name);
			if (candidate != nullptr) return candidate;
		}
		return nullptr;
	}
	any any::value_conv(std::string_view base_name) const
	{
		const auto data = type_data();

		/* If `this` is directly convertible to `name`, use the existing conversion. */
		if (const auto *conv = data->find_conv(base_name, *m_db); conv != nullptr)
			return (*conv)(cdata());

		/* Otherwise, recursively convert through a base type. */
		for (auto [_, base]: data->bases)
		{
			const auto *base_ptr = base.cast_func(cdata());
			const auto base_type = type_info{base.type, *m_db};
			auto candidate = any{base_type, base_ptr}.value_conv(base_name);
			if (!candidate.empty()) return candidate;
		}
		return {};
	}

	void any::destroy()
	{
		/* Bail if empty or non-owning. */
		if (empty() || is_ref()) return;

		if (!(flags() & detail::is_value))
			(*deleter())(external());
	}

	bool any::operator==(const any &other) const
	{
		const auto cmp = facet<facets::compare>();
		return empty() == other.empty() || (cmp.vtable() && cmp.cmp_eq(other));
	}
	bool any::operator!=(const any &other) const
	{
		const auto cmp = facet<facets::compare>();
		return empty() != other.empty() || (cmp.vtable() && cmp.cmp_ne(other));
	}
	bool any::operator>=(const any &other) const
	{
		const auto cmp = facet<facets::compare>();
		return empty() <= other.empty() || (cmp.vtable() && cmp.cmp_ge(other));
	}
	bool any::operator<=(const any &other) const
	{
		const auto cmp = facet<facets::compare>();
		return empty() <= other.empty() || (cmp.vtable() && cmp.cmp_le(other));
	}
	bool any::operator>(const any &other) const
	{
		const auto cmp = facet<facets::compare>();
		return (!empty() && other.empty()) || (cmp.vtable() && cmp.cmp_gt(other));
	}
	bool any::operator<(const any &other) const
	{
		const auto cmp = facet<facets::compare>();
		return (empty() && !other.empty()) || (cmp.vtable() && cmp.cmp_lt(other));
	}
}