/*
 * Created by switchblade on 2023-03-19.
 */

#pragma once

#include <vector>

#include "fwd.hpp"

namespace reflex
{
	/** Exception type thrown when dynamic invocation of a constructor or assignment operator fails due to invalid arguments. */
	class REFLEX_PUBLIC bad_argument_list : public std::runtime_error
	{
	public:
		bad_argument_list(const bad_argument_list &) = default;
		bad_argument_list &operator=(const bad_argument_list &) = default;
		bad_argument_list(bad_argument_list &&) = default;
		bad_argument_list &operator=(bad_argument_list &&) = default;

		/** Initializes the argument list exception with message \a msg. */
		explicit bad_argument_list(const char *msg) : std::runtime_error(msg) {}
		/** @copydoc bad_argument_list */
		explicit bad_argument_list(const std::string &msg) : std::runtime_error(msg) {}

		~bad_argument_list() override = default;
	};

	/** Handle to reflected type information. */
	class type_info
	{
		friend class any;

	public:
		/** Returns type info for type with name \a name, or an invalid type info if the type has not been reflected yet. */
		[[nodiscard]] inline static type_info get(std::string_view name);
		/** Returns type info for type \a T. */
		template<typename T>
		[[nodiscard]] inline static type_info get();

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

		/** Checks if the referenced type is a pointer type.
		 * @note For pointer-like class types, check the `pointer_like` facet. */
		[[nodiscard]] constexpr bool is_pointer() const noexcept;
		/** Checks if the referenced type is an integral type, or can be converted to `std::intmax_t` or `std::uintmax_t`. */
		[[nodiscard]] constexpr bool is_integral() const noexcept;
		/** Checks if the referenced type is a signed integral type, or can be converted to `std::intmax_t`. */
		[[nodiscard]] constexpr bool is_signed_integral() const noexcept;
		/** Checks if the referenced type is an unsigned integral type, or can be converted to `std::uintmax_t`. */
		[[nodiscard]] constexpr bool is_unsigned_integral() const noexcept;
		/** Checks if the referenced type is an arithmetic type, or can be converted to `double`. */
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

		/** Checks if the referenced type inherits from a base type \a T. */
		template<typename T>
		[[nodiscard]] bool inherits_from() const { return inherits_from(type_name_v<T>); }
		/** Checks if the referenced type inherits from a base type \a type. */
		[[nodiscard]] bool inherits_from(type_info type) const noexcept { return inherits_from(type.name()); }
		/** Checks if the referenced type inherits from a base type with name \a name. */
		[[nodiscard]] REFLEX_PUBLIC bool inherits_from(std::string_view name) const noexcept;

		/** Checks if the referenced type is convertible to type \a T, or inherits from a type convertible to \a T. */
		template<typename T>
		[[nodiscard]] bool convertible_to() const { return convertible_to(type_name_v<T>); }
		/** Checks if the referenced type is convertible to type \a type, or inherits from a type convertible to \a type. */
		[[nodiscard]] bool convertible_to(type_info type) const noexcept { return convertible_to(type.name()); }
		/** Checks if the referenced type is convertible to type with name \a name, or inherits from a type convertible to \a name. */
		[[nodiscard]] REFLEX_PUBLIC bool convertible_to(std::string_view name) const noexcept;

		/** Checks if the referenced type is same as, inherits from, or can be type-casted to type \a T. */
		template<typename T>
		[[nodiscard]] bool compatible_with() const { return compatible_with(type_name_v<T>); }
		/** Checks if the referenced type is same as, inherits from, or can be type-casted to type \a type. */
		[[nodiscard]] bool compatible_with(type_info type) const noexcept { return *this == type || inherits_from(type) || convertible_to(type); }
		/** Checks if the referenced type is same as, inherits from, or can be type-casted to type with name \a name. */
		[[nodiscard]] bool compatible_with(std::string_view name) const noexcept { return this->name() == name || inherits_from(name) || convertible_to(name); }

		/** Checks if the referenced type has a property with name \a name. */
		[[nodiscard]] REFLEX_PUBLIC bool has_property(std::string_view name) const noexcept;

		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept = default;
		[[nodiscard]] constexpr bool operator!=(const type_info &other) const noexcept = default;

	private:
		/* Convenience operator for access to underlying type_data. */
		[[nodiscard]] constexpr const detail::type_data *operator->() const noexcept { return m_data; }

		inline void fill_parents(tpp::dense_set<type_info, detail::str_hash, detail::str_cmp> &result) const;

		const detail::type_data *m_data = nullptr;
		detail::database_impl *m_db = nullptr;
	};

	[[nodiscard]] constexpr bool operator==(const type_info &a, const std::string_view &b) noexcept { return a.name() == b; }
	[[nodiscard]] constexpr bool operator==(const std::string_view &a, const type_info &b) noexcept { return a == b.name(); }
	[[nodiscard]] constexpr bool operator!=(const type_info &a, const std::string_view &b) noexcept { return a.name() == b; }
	[[nodiscard]] constexpr bool operator!=(const std::string_view &a, const type_info &b) noexcept { return a == b.name(); }

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
