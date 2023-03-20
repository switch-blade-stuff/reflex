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
		template<typename = void, typename = void, typename = void, typename...>
		class type_ctor;

		struct argument_data;
		struct type_data;
	}

	class type_constructor_error;
	class facet_function_error;

	class type_domain;
	class argument_info;
	class type_info;
	class any;

	template<typename Vtable>
	class facet;
	template<typename... Fs>
	class facet_group;
}