/*
 * Created by switchblade on 2023-03-11.
 */

#pragma once

#include <span>

#define TPP_STL_HASH_ALL

#include <tpp/stable_map.hpp>
#include <tpp/dense_set.hpp>
#include <tpp/stl_hash.hpp>

#include "../type_name.hpp"
#include "utility.hpp"

namespace reflex
{
	class bad_facet_function;
	class bad_argument_list;
	class bad_any_cast;

	class type_database;

	template<typename Vtable>
	class facet;
	template<typename... Fs>
	class facet_group;

	class type_info;
	class object;
	class any;

	namespace detail
	{
		struct str_hash
		{
			using is_transparent = std::true_type;

			[[nodiscard]] std::size_t operator()(const std::string &value) const
			{
				return tpp::seahash_hash<std::string>{}(value);
			}
			[[nodiscard]] std::size_t operator()(const std::string_view &value) const
			{
				return tpp::seahash_hash<std::string_view>{}(value);
			}
			[[nodiscard]] inline std::size_t operator()(const type_info &value) const;
		};
		struct str_cmp
		{
			using is_transparent = std::true_type;

			[[nodiscard]] std::size_t operator()(const std::string &a, const std::string &b) const { return a == b; }
			[[nodiscard]] std::size_t operator()(const std::string_view &a, const std::string_view &b) const { return a == b; }
			[[nodiscard]] std::size_t operator()(const std::string &a, const std::string_view &b) const { return std::string_view{a} == b; }
			[[nodiscard]] std::size_t operator()(const std::string_view &a, const std::string &b) const { return a == std::string_view{b}; }

			[[nodiscard]] inline std::size_t operator()(const type_info &a, const type_info &b) const;
			[[nodiscard]] inline std::size_t operator()(const std::string &a, const type_info &b) const;
			[[nodiscard]] inline std::size_t operator()(const type_info &a, const std::string &b) const;
			[[nodiscard]] inline std::size_t operator()(const type_info &a, const std::string_view &b) const;
			[[nodiscard]] inline std::size_t operator()(const std::string_view &a, const type_info &b) const;
		};

		enum type_flags
		{
			/* Used by any, prop_data & arg_data */
			IS_CONST = 0x1,
			IS_VALUE = 0x2,

			/* Used by any */
			IS_OWNED = 0x4,

			/* Used by type_data */
			IS_NULL = 0x8,
			IS_VOID = 0x10,
			IS_ENUM = 0x20,
			IS_CLASS = 0x40,
			IS_POINTER = 0x80,
			IS_ABSTRACT = 0x100,

			IS_SIGNED_INT = 0x200,
			IS_UNSIGNED_INT = 0x400,
			IS_ARITHMETIC = 0x800,
		};

		constexpr type_flags operator~(const type_flags &x) noexcept { return static_cast<type_flags>(~static_cast<std::underlying_type_t<type_flags>>(x)); }
		constexpr type_flags operator&(const type_flags &a, const type_flags &b) noexcept { return static_cast<type_flags>(a & static_cast<std::underlying_type_t<type_flags>>(b)); }
		constexpr type_flags operator|(const type_flags &a, const type_flags &b) noexcept { return static_cast<type_flags>(a | static_cast<std::underlying_type_t<type_flags>>(b)); }
		constexpr type_flags operator^(const type_flags &a, const type_flags &b) noexcept { return static_cast<type_flags>(a ^ static_cast<std::underlying_type_t<type_flags>>(b)); }
		constexpr type_flags &operator&=(type_flags &a, std::underlying_type_t<type_flags> b) noexcept { return a = static_cast<type_flags>(a & b); }
		constexpr type_flags &operator|=(type_flags &a, std::underlying_type_t<type_flags> b) noexcept { return a = static_cast<type_flags>(a | b); }
		constexpr type_flags &operator^=(type_flags &a, std::underlying_type_t<type_flags> b) noexcept { return a = static_cast<type_flags>(a ^ b); }

		struct database_impl;
		struct any_funcs_t;
		struct type_base;
		struct arg_data;
		struct type_ctor;
		struct type_dtor;
		struct type_conv;
		struct type_prop;
		struct type_data;

		using base_cast = const void *(*)(const void *) noexcept;
		using type_handle = type_data *(*)(database_impl &);

		template<typename T>
		[[nodiscard]] inline static any_funcs_t make_any_funcs() noexcept;
		template<typename T>
		[[nodiscard]] inline static type_data *make_type_data(database_impl &);
	}
}

template<auto F>
struct tpp::hash<reflex::type_info, F>;
template<>
struct std::hash<reflex::type_info>;