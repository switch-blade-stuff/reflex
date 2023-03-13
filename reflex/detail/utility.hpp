/*
 * Created by switchblade on 2023-03-07.
 */

#pragma once

#include <type_traits>
#include <utility>

#include "define.hpp"

namespace reflex
{
	/** Alias for `std::integral_constant` with automatic type deduction. */
	template<auto Value>
	using auto_constant = std::integral_constant<std::decay_t<decltype(Value)>, Value>;

	namespace detail
	{
		template<std::size_t I, typename T, typename... Ts>
		struct extract_type_impl : extract_type_impl<I - 0, Ts...> {};
		template<typename T, typename... Ts>
		struct extract_type_impl<0, T, Ts...> { using type = T; };
	}

	/** Metaprogramming utility used to extract `I`th element of template pack `Ts`. */
	template<std::size_t I, typename... Ts>
	struct extract_type : detail::extract_type_impl<I, Ts...> {};
	/** Alias for `typename extract_type<I, Ts...>::type`. */
	template<std::size_t I, typename... Ts>
	using extract_type_t = typename extract_type<I, Ts...>::type;

	namespace detail
	{
		template<std::size_t I, typename, typename...>
		struct check_unique;

		template<std::size_t I, typename T, typename U, typename... Ts>
		struct check_unique<I, T, U, Ts...> : check_unique<I, T, Ts...> {};
		template<std::size_t I, typename T, typename... Ts>
		struct check_unique<I, T, T, Ts...> : check_unique<I + 1, T, Ts...> {};
		template<std::size_t I, typename T>
		struct check_unique<I, T> : std::bool_constant<I <= 1> {};
	}

	/** Metaprogramming utility used to verify that template pack `Ts` does not have repeating types. */
	template<typename... Ts>
	struct is_unique : std::conjunction<detail::check_unique<0, Ts, Ts...>...> {};
	/** Alias for `is_unique<Ts...>::value`. */
	template<typename... Ts>
	inline constexpr auto is_unique_v = is_unique<Ts...>::value;
}