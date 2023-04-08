/*
 * Created by switchblade on 2023-03-07.
 */

#pragma once

#include <type_traits>
#include <concepts>
#include <iterator>
#include <utility>
#include <memory>

#include "detail/define.hpp"

namespace reflex
{
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
		template<typename T, typename = void>
		struct has_tuple_size : std::false_type {};
		template<typename T>
		struct has_tuple_size<T, std::void_t<decltype(sizeof(std::tuple_size<T>))>> : std::true_type {};
		template<typename T, typename = void>
		struct has_tuple_element : std::false_type {};
		template<typename T>
		struct has_tuple_element<T, std::void_t<decltype(sizeof(std::tuple_element<std::size_t{}, T>))>> : std::true_type {};
	}

	/** Concept used to check if a type is tuple-like. */
	template<typename T>
	concept tuple_like = requires(T v)
	{
		detail::has_tuple_element<T>::value;
		detail::has_tuple_size<T>::value;
		get<std::size_t{}>(v);
	};
	/** Concept used to check if a type is pair-like (tuple-like type of size 2). */
	template<typename T>
	concept pair_like = requires(T v)
	{
		tuple_like<T>;
		std::tuple_size_v<T> == 2;
		get<0>(v);
		get<1>(v);
	};

	/** Concept used to check if a type is pointer or pointer-like. */
	template<typename T>
	concept pointer_like = std::is_pointer_v<T> || requires(T v)
	{
		typename std::pointer_traits<T>;
		{ *v } -> std::common_reference_with<typename std::pointer_traits<T>::element_type>;
		std::to_address(v);
	};

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

	/** Metaprogramming utility used to group a pack of types \a Ts. */
	template<typename... Ts>
	struct type_pack_t {};
	/** Instance of `type_pack_t<Ts...>`. */
	template<typename... Ts>
	inline constexpr auto type_pack = type_pack_t<Ts...>{};

	namespace detail
	{
		template<typename>
		struct impl_template_pack;
		template<template<typename...> typename T, typename... Ts>
		struct impl_template_pack<T<Ts...>> { using type = type_pack_t<Ts...>; };
	}

	/** Metaprogramming utility used extract type pack of template instance \a T as an instance of `type_pack_t`. */
	template<typename T>
	using template_pack_t = typename detail::impl_template_pack<T>::type;
	/** Instance of `template_pack_t<T>`. */
	template<typename T>
	inline constexpr auto template_pack = template_pack_t<T>{};

	namespace detail
	{
		template<typename, typename...>
		struct is_in_impl;

		template<typename U, typename T, typename... Ts>
		struct is_in_impl<U, T, Ts...> : is_in_impl<U, Ts...> {};
		template<typename U, typename... Ts>
		struct is_in_impl<U, U, Ts...> : std::true_type {};
		template<typename T>
		struct is_in_impl<T> : std::false_type {};

		template<typename, typename>
		struct unique_type_pack_impl;
		template<typename... Ts, typename U, typename... Us> requires(is_in_impl<U, Ts...>::value)
		struct unique_type_pack_impl<type_pack_t<Ts...>, type_pack_t<U, Us...>> : unique_type_pack_impl<type_pack_t<Ts...>, type_pack_t<Us...>> {};
		template<typename... Ts, typename U, typename... Us> requires (!is_in_impl<U, Ts...>::value)
		struct unique_type_pack_impl<type_pack_t<Ts...>, type_pack_t<U, Us...>> : unique_type_pack_impl<type_pack_t<U, Ts...>, type_pack_t<Us...>> {};
		template<typename... Ts>
		struct unique_type_pack_impl<type_pack_t<Ts...>, type_pack_t<>> { using type = type_pack_t<Ts...>; };
	}

	/** Metaprogramming utility used to filter duplicate types from type pack `P`. */
	template<typename P>
	using unique_type_pack_t = typename detail::unique_type_pack_impl<type_pack_t<>, P>::type;
	/** Instance of `unique_type_pack_t<P>`. */
	template<typename P>
	inline constexpr auto unique_type_pack = unique_type_pack_t<P>{};
}