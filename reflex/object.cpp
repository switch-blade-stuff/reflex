/*
 * Created by switchblade on 2023-03-25.
 */

#ifndef REFLEX_HEADER_ONLY

#include "object.hpp"

namespace reflex
{
	object::~object() = default;

	object *detail::checked_object_cast(object *ptr, type_info from, type_info to) noexcept
	{
		if (from == to || from.inherits_from(to))
			return ptr;
		else
			return nullptr;
	}
}
#endif