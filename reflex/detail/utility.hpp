/*
 * Created by switchblade on 2023-03-07.
 */

#pragma once

#include <type_traits>
#include <iterator>
#include <utility>

#include "define.hpp"

namespace reflex
{
	/** Metaprogramming utility used to group a pack of types \a Ts. */
	template<typename... Ts>
	struct type_pack_t {};
	/** Instance of `type_pack_t<Ts...>`. */
	template<typename... Ts>
	inline constexpr auto type_pack = type_pack_t<Ts...>{};

	/** Metaprogramming utility used to take const qualifier from \a From and apply it to \a To. */
	template<typename To, typename From>
	struct take_const { using type = std::conditional_t<std::is_const_v<From>, std::add_const_t<To>, To>; };
	/** Alias for `typename take_const<To, From>::type`. */
	template<typename To, typename From>
	using take_const_t = typename take_const<To, From>::type;

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

	namespace detail
	{
		template<typename T>
		[[nodiscard]] inline static T *void_cast(void *ptr) noexcept
		{
			if constexpr (std::is_const_v<T>)
				return static_cast<T *>(const_cast<const void *>(ptr));
			else
				return static_cast<T *>(ptr);
		}
		template<typename T>
		[[nodiscard]] inline static T *void_cast(const void *ptr) noexcept
		{
			if constexpr (!std::is_const_v<T>)
				return static_cast<T *>(const_cast<void *>(ptr));
			else
				return static_cast<T *>(ptr);
		}

		/* CLang has issues with std::ranges::subrange. As such, define our own. */
		template<typename Iter, typename Sent = Iter>
		class subrange
		{
		public:
			constexpr subrange(const Iter &iter, const Sent &sent) : m_iter(iter), m_sent(sent) {}
			constexpr subrange(Iter &&iter, Sent &&sent) : m_iter(std::forward<Iter>(iter)), m_sent(std::forward<Sent>(sent)) {}

			[[nodiscard]] constexpr Iter begin() const { return m_iter; }
			[[nodiscard]] constexpr Sent end() const { return m_sent; }

			[[nodiscard]] constexpr decltype(auto) front() const { return *m_iter; }
			[[nodiscard]] constexpr decltype(auto) back() const { return *std::prev(m_sent); }
			[[nodiscard]] constexpr decltype(auto) operator[](std::iter_difference_t<Iter> i) const { return m_iter[i]; }

			[[nodiscard]] constexpr bool empty() const { return m_iter == m_sent; }
			[[nodiscard]] constexpr auto size() const
			{
				const auto diff = std::distance(m_iter, m_sent);
				return static_cast<std::make_unsigned_t<std::decay_t<decltype(diff)>>>(diff);
			}

		private:
			Iter m_iter;
			Sent m_sent;
		};

		template<typename, template<typename...> typename>
		struct template_instance_impl : std::false_type {};
		template<template<typename...> typename T, typename... Ts>
		struct template_instance_impl<T<Ts...>, T> : std::true_type {};
	}

	/** Concept used to check if \a I is an instance of template \a T. */
	template<typename I, template<typename...> typename T>
	concept template_instance = detail::template_instance_impl<I, T>::value;
}