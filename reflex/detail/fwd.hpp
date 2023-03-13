/*
 * Created by switchblade on 2023-03-11.
 */

#pragma once

#include "../type_name.hpp"
#include "utility.hpp"

namespace reflex
{
	namespace detail
	{
		struct type_data;
	}

	class type_info;
	class any;

	template<typename Vtable>
	class facet;
	template<typename... Fs>
	class facet_group;

	class facet_function_error;
}