/*
 * Created by switchblade on 2023-03-11.
 */

#pragma once

#include <concepts>
#include <ranges>
#include <span>

#define TPP_STL_HASH_ALL
#include <tpp/stable_map.hpp>
#include <tpp/dense_map.hpp>
#include <tpp/dense_set.hpp>
#include <tpp/stl_hash.hpp>

#include "../type_name.hpp"
#include "utility.hpp"

namespace reflex
{
	namespace facets
	{
		class bad_facet_function;
		template<typename Vtable>
		class facet;
		template<typename... Fs>
		class facet_group;
	}

	class bad_any_copy;
	class bad_any_cast;

	class type_database;

	template<typename T>
	class type_factory;
	template<typename T>
	struct type_init;

	template<typename...>
	class type_query;

	class constructor_view;
	class constructor_info;
	class argument_info;
	class argument_view;
	class type_info;
	class object;
	class any;

	namespace detail
	{
		struct type_hash
		{
			using is_transparent = std::true_type;

			[[nodiscard]] std::size_t operator()(const std::string &value) const { return tpp::seahash_hash<std::string>{}(value); }
			[[nodiscard]] std::size_t operator()(const std::string_view &value) const { return tpp::seahash_hash<std::string_view>{}(value); }
			[[nodiscard]] inline std::size_t operator()(const type_info &value) const;
		};
		struct type_eq
		{
			using is_transparent = std::true_type;

			[[nodiscard]] bool operator()(const std::string &a, const std::string &b) const { return a == b; }
			[[nodiscard]] bool operator()(const std::string_view &a, const std::string_view &b) const { return a == b; }
			[[nodiscard]] bool operator()(const std::string &a, const std::string_view &b) const { return std::string_view{a} == b; }
			[[nodiscard]] bool operator()(const std::string_view &a, const std::string &b) const { return a == std::string_view{b}; }

			[[nodiscard]] inline bool operator()(const type_info &a, const type_info &b) const;
			[[nodiscard]] inline bool operator()(const std::string &a, const type_info &b) const;
			[[nodiscard]] inline bool operator()(const type_info &a, const std::string &b) const;
			[[nodiscard]] inline bool operator()(const type_info &a, const std::string_view &b) const;
			[[nodiscard]] inline bool operator()(const std::string_view &a, const type_info &b) const;
		};

		using type_set = tpp::dense_set<type_info, detail::type_hash, detail::type_eq>;
		using enum_map = tpp::dense_map<std::string_view, any>;
		using attr_map = tpp::dense_map<type_info, any>;

		enum type_flags
		{
			/* Used by any, prop_data & arg_data */
			is_const = 0x1,
			is_value = 0x2,

			/* Used by any */
			is_owned = 0x4,
			any_flags_max = 0x7,

			/* Used by type_data */
			is_null = 0x8,
			is_void = 0x10,
			is_enum = 0x20,
			is_class = 0x40,
			is_pointer = 0x80,
			is_abstract = 0x100,

			is_signed_int = 0x200,
			is_unsigned_int = 0x400,
			is_arithmetic = 0x800,
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
		struct arg_data;
		struct type_base;
		struct type_ctor;
		struct type_conv;
		struct type_data;

		using base_cast = const void *(*)(const void *) noexcept;
		using type_handle = type_data *(*)(database_impl &);

		template<typename T>
		[[nodiscard]] constexpr static any_funcs_t make_any_funcs() noexcept;
		template<typename T>
		[[nodiscard]] inline static type_data *make_type_data(database_impl &);
	}
}

template<auto F>
struct tpp::hash<reflex::type_info, F>;
template<>
struct std::hash<reflex::type_info>;