/*
 * Created by switchblade on 2023-03-19.
 */

#pragma once

#include "type_domain.hpp"

namespace reflex
{
	/** Handle to reflected type information. */
	class type_info
	{
		friend class argument_info;
		friend class type_domain;
		friend class any;

	public:
		/** Returns (potentially inserting) type info for type \a T from domain \a domain. */
		template<typename T>
		[[nodiscard]] static type_info get(type_domain &domain) noexcept { return {detail::type_data::bind<T>, domain}; }
		/** Returns type info for type \a T from the global domain. */
		template<typename T>
		[[nodiscard]] static type_info get() noexcept { return get<T>(*type_domain::instance().access()); }

	private:
		type_info(detail::type_handle handle, type_domain &domain) noexcept : type_info(handle(domain), &domain) {}
		constexpr type_info(const detail::type_data *data, type_domain *domain) noexcept : m_data(data), m_domain(domain) {}

	public:
		/** Initializes an invalid type info. */
		constexpr type_info() noexcept = default;

		/** Checks if the type info references a valid type. */
		[[nodiscard]] constexpr bool valid() const noexcept { return m_data != nullptr; }
		/** @copydoc valid */
		[[nodiscard]] constexpr operator bool() const noexcept { return valid(); }

		/** Returns access guard to the domain of the referenced type. */
		[[nodiscard]] constexpr auto domain() const noexcept { return type_domain::guard(m_domain); }
		/** Returns the name of the referenced type. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return m_data->name; }
		/** Returns the size of the referenced type.
		 * @note If `is_empty_v<T>` evaluates to `true` for referenced type \a T, returns `0` instead of `1`. */
		[[nodiscard]] constexpr std::size_t size() const noexcept { return m_data->size; }

		/** Checks if the referenced type is empty. */
		[[nodiscard]] constexpr bool is_empty() const noexcept { return size() == 0; }

		/** Checks if the referenced type is `void`. */
		[[nodiscard]] constexpr bool is_void() const noexcept { return m_data->flags & detail::IS_VOID; }
		/** Checks if the referenced type is `std::nullptr_t`. */
		[[nodiscard]] constexpr bool is_nullptr() const noexcept { return m_data->flags & detail::IS_NULL; }

		/** Checks if the referenced type is an enum. */
		[[nodiscard]] constexpr bool is_enum() const noexcept { return m_data->flags & detail::IS_ENUM; }
		/** Checks if the referenced type is a class. */
		[[nodiscard]] constexpr bool is_class() const noexcept { return m_data->flags & detail::IS_CLASS; }

		/** Checks if the referenced type is a pointer type.
		 * @note For pointer-like class types, check the `pointer_like` facet. */
		[[nodiscard]] constexpr bool is_pointer() const noexcept { return m_data->flags & detail::IS_POINTER; }
		/** Checks if the referenced type is an integral type, or can be converted to `std::intmax_t` or `std::uintmax_t`. */
		[[nodiscard]] constexpr bool is_integral() const noexcept { return m_data->flags & (detail::IS_SIGNED_INT | detail::IS_UNSIGNED_INT); }
		/** Checks if the referenced type is a signed integral type, or can be converted to `std::intmax_t`. */
		[[nodiscard]] constexpr bool is_signed_integral() const noexcept { return m_data->flags & detail::IS_SIGNED_INT; }
		/** Checks if the referenced type is an unsigned integral type, or can be converted to `std::uintmax_t`. */
		[[nodiscard]] constexpr bool is_unsigned_integral() const noexcept { return m_data->flags & detail::IS_UNSIGNED_INT; }
		/** Checks if the referenced type is an arithmetic type, or can be converted to `double`. */
		[[nodiscard]] constexpr bool is_arithmetic() const noexcept { return m_data->flags & detail::IS_ARITHMETIC; }

		/** Removes pointer from the referenced type. */
		[[nodiscard]] type_info remove_pointer() const noexcept { return {m_data->remove_pointer, *domain().access()}; }
		/** Removes pointer from the referenced type using type domain \a domain. */
		[[nodiscard]] type_info remove_pointer(type_domain &domain) const noexcept { return {m_data->remove_pointer, domain}; }
		/** Removes extent from the referenced type. */
		[[nodiscard]] type_info remove_extent() const noexcept { return {m_data->remove_extent, *domain().access()}; }
		/** Removes extent from the referenced type using type domain \a domain. */
		[[nodiscard]] type_info remove_extent(type_domain &domain) const noexcept { return {m_data->remove_extent, domain}; }

		/** Returns the extent of the referenced type. */
		[[nodiscard]] constexpr std::size_t extent() const noexcept { return m_data->extent; }
		/** Checks if the referenced type is an array type. */
		[[nodiscard]] constexpr bool is_array() const noexcept { return extent() > 0; }

		[[nodiscard]] constexpr bool operator!=(const type_info &other) const noexcept = default;
		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept = default;

	private:
		const detail::type_data *m_data = nullptr;
		type_domain *m_domain = nullptr;
	};
}