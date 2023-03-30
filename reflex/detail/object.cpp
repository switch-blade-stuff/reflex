/*
 * Created by switchblade on 2023-03-25.
 */

#include "object.hpp"

namespace reflex
{
	object::~object() = default;

	void any::throw_bad_any_cast(type_info from_type, type_info to_type) { throw bad_any_cast(from_type, to_type); }
	void any::throw_bad_any_copy(type_info type) { throw bad_any_copy(type); }

	bad_facet_function::~bad_facet_function() = default;
	bad_any_copy::~bad_any_copy() = default;
	bad_any_cast::~bad_any_cast() = default;
}