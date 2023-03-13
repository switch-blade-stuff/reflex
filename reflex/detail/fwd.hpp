/*
 * Created by switchblade on 2023-03-11.
 */

#pragma once

#include "utility.hpp"

namespace reflex
{
	class any;

	template<typename Vtable>
	class facet;
	template<typename... Fs>
	class facet_group;

	class unbound_facet_error;
}