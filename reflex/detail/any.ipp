/*
 * Created by switchblade on 2023-03-22.
 */

#include "database.hpp"
#include "any.hpp"

namespace reflex
{
	any any::try_cast(type_info type)
	{
		/* If `type` is the same as or is a base of `this`, return reference to `this`. */
		if (type == m_type) return ref();
		/* If `this` is convertible to `type`, convert `this` to `type`. */
		if (const auto *base_ptr = base_cast(type); base_ptr != nullptr)
		{
			if (!is_const())
				return any{type, const_cast<void *>(base_ptr)};
			else
				return any{type, base_ptr};
		}
		/* Otherwise, attempt to convert by-value. */
		return value_conv(type);
	}
	any any::try_cast(type_info type) const
	{
		/* If `type` is the same as or is a base of `this`, return reference to `this`. */
		if (type == m_type) return ref();
		/* If `this` is convertible to `type`, convert `this` to `type`. */
		if (const auto *base_ptr = base_cast(type); base_ptr != nullptr)
			return any{type, base_ptr};
		/* Otherwise, attempt to convert by-value. */
		return value_conv(type);
	}

	const void *any::base_cast(type_info type) const
	{
		/* If `type` is an immediate base of `this`, attempt to cast through that. */
		if (const auto *base = m_type->find_base(type.name()); base != nullptr)
			return base->cast_func(cdata());

		/* Otherwise, recursively cast through a base type. */
		for (auto [_, base]: m_type->base_list)
		{
			const auto *base_ptr = base.cast_func(cdata());
			const auto base_type = type_info{base.type, *m_type.m_db};
			auto candidate = any{base_type, base_ptr}.base_cast(type);
			if (candidate != nullptr) return candidate;
		}
		return nullptr;
	}
	any any::value_conv(type_info type) const
	{
		/* If `this` is directly convertible to `name`, use the existing conversion. */
		if (const auto *conv = m_type->find_conv(type.name()); conv != nullptr)
			return conv->conv_func(cdata());

		/* Otherwise, recursively convert through a base type. */
		for (auto [_, base]: m_type->base_list)
		{
			const auto *base_ptr = base.cast_func(cdata());
			const auto base_type = type_info{base.type, *m_type.m_db};
			auto candidate = any{base_type, base_ptr}.value_conv(type);
			if (!candidate.empty()) return candidate;
		}
		return {};
	}

	void any::destroy()
	{
		/* Bail if empty or non-owning. */
		if (empty() || is_ref()) return;

		if (flags() & detail::IS_VALUE)
			m_type->dtor.placement_func(local());
		else if (external().deleter == nullptr)
			m_type->dtor.deleting_func(external().data);
		else
		{
			m_type->dtor.placement_func(external().data);
			external().deleter(external().data);
		}
	}
}