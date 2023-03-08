/*
 * Created by switchblade on 2023-03-07.
 */

#pragma once

#include "define.hpp"

#include <type_traits>

namespace reflex::detail
{
	template<auto Value>
	using auto_constant = std::integral_constant<std::remove_cvref_t<decltype(Value)>, Value>;
}