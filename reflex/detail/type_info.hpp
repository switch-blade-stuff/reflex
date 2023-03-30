/*
 * Created by switchblade on 2023-03-19.
 */

#pragma once

#include <vector>

#include "fwd.hpp"

namespace reflex
{
	/** Handle to reflected type information. */
	class type_info
	{
		template<typename>
		friend class type_factory;
		friend class any;

	public:
		/** Reflects type info for type \a T and returns a type factory used for initialization.
		 * @note Modifications of type info made through the type factory are not thread-safe and must be synchronized externally. */
		template<typename T>
		inline static type_factory<T> reflect();

		/** Returns type info for type with name \a name, or an invalid type info if the type has not been reflected yet. */
		[[nodiscard]] inline static type_info get(std::string_view name);
		/** Returns type info for type \a T. */
		template<typename T>
		[[nodiscard]] inline static type_info get();

		/** Resets type info for type with name \a name to it's initial state. */
		inline static void reset(std::string_view name);
		/** Resets type info for type \a T to it's initial state. */
		template<typename T>
		inline static void reset();
		/** Resets all reflected `type_info`s to their initial state. */
		inline static void reset();

	private:
		type_info(detail::type_handle handle, detail::database_impl &db) : m_data(handle(db)), m_db(&db) {}
		constexpr type_info(const detail::type_data *data, detail::database_impl *db) noexcept : m_data(data), m_db(db) {}

	public:
		/** Initializes an invalid type info. */
		constexpr type_info() noexcept = default;

		/** Checks if the type info references a valid type. */
		[[nodiscard]] constexpr bool valid() const noexcept { return m_data != nullptr; }
		/** @copydoc valid */
		[[nodiscard]] constexpr operator bool() const noexcept { return valid(); }

		/** Returns the name of the referenced type. */
		[[nodiscard]] constexpr std::string_view name() const noexcept;

		/** Returns the size of the referenced type.
		 * @note If `is_empty_v<T>` evaluates to `true` for referenced type \a T, returns `0` instead of `1`. */
		[[nodiscard]] constexpr std::size_t size() const noexcept;
		/** Returns the alignment of the referenced type. */
		[[nodiscard]] constexpr std::size_t alignment() const noexcept;

		/** Checks if the referenced type is `void`. */
		[[nodiscard]] constexpr bool is_void() const noexcept;
		/** Checks if the referenced type is empty. */
		[[nodiscard]] constexpr bool is_empty() const noexcept;
		/** Checks if the referenced type is `std::nullptr_t`. */
		[[nodiscard]] constexpr bool is_nullptr() const noexcept;

		/** Checks if the referenced type is an enum. */
		[[nodiscard]] constexpr bool is_enum() const noexcept;
		/** Checks if the referenced type is a class. */
		[[nodiscard]] constexpr bool is_class() const noexcept;
		/** Checks if the referenced type is an abstract type. */
		[[nodiscard]] constexpr bool is_abstract() const noexcept;

		/** Checks if the referenced type is a pointer type.
		 * @note For pointer-like class types, check the `pointer_like` facet. */
		[[nodiscard]] constexpr bool is_pointer() const noexcept;
		/** Checks if the referenced type is an integral type.
		 * @note For conversions to integral types, use `convertible_to`. */
		[[nodiscard]] constexpr bool is_integral() const noexcept;
		/** Checks if the referenced type is a signed integral type.
		 * @note For conversions to signed integral types, use `convertible_to`. */
		[[nodiscard]] constexpr bool is_signed_integral() const noexcept;
		/** Checks if the referenced type is an unsigned integral type.
		 * @note For conversions to unsigned integral types, use `convertible_to`. */
		[[nodiscard]] constexpr bool is_unsigned_integral() const noexcept;
		/** Checks if the referenced type is an arithmetic type.
		 * @note For conversions to arithmetic types, use `convertible_to`. */
		[[nodiscard]] constexpr bool is_arithmetic() const noexcept;

		/** Removes extent from the referenced type. */
		[[nodiscard]] inline type_info remove_extent() const noexcept;
		/** Removes pointer from the referenced type. */
		[[nodiscard]] inline type_info remove_pointer() const noexcept;

		/** Checks if the referenced type is an array type. */
		[[nodiscard]] constexpr bool is_array() const noexcept;
		/** Returns the extent of the referenced type. */
		[[nodiscard]] constexpr std::size_t extent() const noexcept;

		/** Returns a set of the referenced type's parents (including the parents' parents). */
		[[nodiscard]] REFLEX_PUBLIC auto parents() const -> tpp::dense_set<type_info, detail::str_hash, detail::str_cmp>;
		/** Returns a map of the referenced type's enumerations. */
		[[nodiscard]] REFLEX_PUBLIC auto enumerations() const -> tpp::dense_map<std::string_view, any, detail::str_hash, detail::str_cmp>;

		/** Checks if the referenced type has an enumeration with value \a value. */
		[[nodiscard]] REFLEX_PUBLIC bool has_enumeration(const any &value) const;
		/** Checks if the referenced type has an enumeration with name \a name. */
		[[nodiscard]] REFLEX_PUBLIC bool has_enumeration(std::string_view name) const;

		/** Returns value of the enumeration with name \a name, or an empty `any` if no such enumeration exists. */
		[[nodiscard]] REFLEX_PUBLIC any enumerate(std::string_view name) const;

		/** Checks if the referenced type implements a facet type \a T. */
		template<typename T>
		[[nodiscard]] inline bool implements_facet() const noexcept;
		/** Checks if the referenced type implements all facets in facet group \a G. */
		template<template_instance<facets::facet_group> G>
		[[nodiscard]] inline bool implements_facet() const noexcept;

		/** Checks if the referenced type inherits from a base type \a T. */
		template<typename T>
		[[nodiscard]] bool inherits_from() const noexcept { return inherits_from(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type inherits from a base type \a type. */
		[[nodiscard]] bool inherits_from(type_info type) const noexcept { return type.valid() && inherits_from(type.name()); }
		/** Checks if the referenced type inherits from a base type with name \a name. */
		[[nodiscard]] REFLEX_PUBLIC bool inherits_from(std::string_view name) const noexcept;

		/** Checks if the referenced type is convertible to type \a T, or inherits from a type convertible to \a T. */
		template<typename T>
		[[nodiscard]] bool convertible_to() const noexcept { return convertible_to(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is convertible to type \a type, or inherits from a type convertible to \a type. */
		[[nodiscard]] bool convertible_to(type_info type) const noexcept { return type.valid() && convertible_to(type.name()); }
		/** Checks if the referenced type is convertible to type with name \a name, or inherits from a type convertible to \a name. */
		[[nodiscard]] REFLEX_PUBLIC bool convertible_to(std::string_view name) const noexcept;

		/** Checks if the referenced type is same as, inherits from, or can be type-cast to type \a T. */
		template<typename T>
		[[nodiscard]] bool compatible_with() const noexcept { return compatible_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is same as, inherits from, or can be type-cast to type \a type. */
		[[nodiscard]] bool compatible_with(type_info type) const noexcept { return *this == type || inherits_from(type) || convertible_to(type); }
		/** Checks if the referenced type is same as, inherits from, or can be type-cast to type with name \a name. */
		[[nodiscard]] bool compatible_with(std::string_view name) const noexcept { return this->name() == name || inherits_from(name) || convertible_to(name); }

		/** Checks if the referenced type is directly (without conversion) comparable (using any comparison operator) with type \a T. */
		template<typename T>
		[[nodiscard]] bool comparable_with() const noexcept { return comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable (using any comparison operator) with type \a type. */
		[[nodiscard]] bool comparable_with(type_info type) const noexcept { return comparable_with(type.name()); }
		/** Checks if the referenced type is directly (without conversion) comparable (using any comparison operator) with type with name \a name. */
		[[nodiscard]] REFLEX_PUBLIC bool comparable_with(std::string_view name) const noexcept;

		/** Checks if the referenced type is directly (without conversion) comparable with type \a T using `operator==` and `operator!=`. */
		template<typename T>
		[[nodiscard]] bool eq_comparable_with() const noexcept { return eq_comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable with type \a type using `operator==` and `operator!=`. */
		[[nodiscard]] bool eq_comparable_with(type_info type) const noexcept { return eq_comparable_with(type.name()); }
		/** Checks if the referenced type is directly (without conversion) comparable with type with name \a name using `operator==` and `operator!=`. */
		[[nodiscard]] REFLEX_PUBLIC bool eq_comparable_with(std::string_view name) const noexcept;

		/** Checks if the referenced type is directly (without conversion) comparable with type \a T using `operator>=`. */
		template<typename T>
		[[nodiscard]] bool ge_comparable_with() const noexcept { return ge_comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable with type \a type using `operator>=`. */
		[[nodiscard]] bool ge_comparable_with(type_info type) const noexcept { return ge_comparable_with(type.name()); }
		/** Checks if the referenced type is directly (without conversion) comparable with type with name \a name using `operator>=`. */
		[[nodiscard]] REFLEX_PUBLIC bool ge_comparable_with(std::string_view name) const noexcept;

		/** Checks if the referenced type is directly (without conversion) comparable with type \a T using `operator<=`. */
		template<typename T>
		[[nodiscard]] bool le_comparable_with() const noexcept { return le_comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable with type \a type using `operator<=`. */
		[[nodiscard]] bool le_comparable_with(type_info type) const noexcept { return le_comparable_with(type.name()); }
		/** Checks if the referenced type is directly (without conversion) comparable with type with name \a name using `operator<=`. */
		[[nodiscard]] REFLEX_PUBLIC bool le_comparable_with(std::string_view name) const noexcept;

		/** Checks if the referenced type is directly (without conversion) comparable with type \a T using `operator>`. */
		template<typename T>
		[[nodiscard]] bool gt_comparable_with() const noexcept { return gt_comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable with type \a type using `operator>`. */
		[[nodiscard]] bool gt_comparable_with(type_info type) const noexcept { return gt_comparable_with(type.name()); }
		/** Checks if the referenced type is directly (without conversion) comparable with type with name \a name using `operator>`. */
		[[nodiscard]] REFLEX_PUBLIC bool gt_comparable_with(std::string_view name) const noexcept;

		/** Checks if the referenced type is directly (without conversion) comparable with type \a T using `operator<`. */
		template<typename T>
		[[nodiscard]] bool lt_comparable_with() const noexcept { return lt_comparable_with(type_name_v<std::decay_t<T>>); }
		/** Checks if the referenced type is directly (without conversion) comparable with type \a type using `operator<`. */
		[[nodiscard]] bool lt_comparable_with(type_info type) const noexcept { return lt_comparable_with(type.name()); }
		/** Checks if the referenced type is directly (without conversion) comparable with type with name \a name using `operator<`. */
		[[nodiscard]] REFLEX_PUBLIC bool lt_comparable_with(std::string_view name) const noexcept;

		/** Checks if the referenced type is constructible from arguments \a Args. */
		template<typename... Args>
		[[nodiscard]] inline bool constructible_from() const;
		/** Checks if the referenced type is constructible from arguments \a args. */
		[[nodiscard]] REFLEX_PUBLIC bool constructible_from(std::span<any> args) const;

		/** Constructs an object of the referenced type from arguments \a args.
		 * @return `any` containing the constructed object instance, or an empty `any` if `this` is not valid or the referenced type is not constructible from \a args. */
		[[nodiscard]] REFLEX_PUBLIC any construct(std::span<any> args) const;
		/** @cpoydoc construct */
		template<std::size_t N>
		[[nodiscard]] inline any construct(std::span<any, N> args) const;
		/** @cpoydoc construct */
		template<typename... Args>
		[[nodiscard]] inline any construct(Args &&...args) const;

		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept = default;
		[[nodiscard]] constexpr bool operator!=(const type_info &other) const noexcept = default;

	private:
		/* Convenience operator for access to underlying type_data. */
		[[nodiscard]] constexpr const detail::type_data *operator->() const noexcept { return m_data; }

		[[nodiscard]] REFLEX_PUBLIC bool has_facet_vtable(std::string_view name) const noexcept;
		[[nodiscard]] REFLEX_PUBLIC bool constructible_from(std::span<const detail::arg_data> args) const;
		inline void fill_parents(tpp::dense_set<type_info, detail::str_hash, detail::str_cmp> &result) const;

		const detail::type_data *m_data = nullptr;
		detail::database_impl *m_db = nullptr;
	};

	[[nodiscard]] constexpr bool operator==(const type_info &a, const std::string_view &b) noexcept { return a.name() == b; }
	[[nodiscard]] constexpr bool operator==(const std::string_view &a, const type_info &b) noexcept { return a == b.name(); }
	[[nodiscard]] constexpr bool operator!=(const type_info &a, const std::string_view &b) noexcept { return a.name() == b; }
	[[nodiscard]] constexpr bool operator!=(const std::string_view &a, const type_info &b) noexcept { return a == b.name(); }

	/** Returns the type info of \a T. Equivalent to `type_info::get<T>()`. */
	template<typename T>
	[[nodiscard]] inline type_info type_of(T &&) { return type_info::get<T>(); }

	namespace literals
	{
		/** Equivalent to `type_info::get`. */
		[[nodiscard]] inline type_info operator ""_type(const char *str, std::size_t n) { return type_info::get({str, n}); }
	}
}

template<auto F>
struct tpp::hash<reflex::type_info, F> { std::size_t operator()(const reflex::type_info &value) const { return tpp::hash<std::string_view, F>{}(value.name()); }};
template<>
struct std::hash<reflex::type_info> { std::size_t operator()(const reflex::type_info &value) const { return std::hash<std::string_view>{}(value.name()); }};
