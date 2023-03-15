/*
 * Created by switchblade on 2023-03-13.
 */

#pragma once

#include "detail/type_db.hpp"
#include "detail/facet.hpp"

namespace reflex
{
	class type_info
	{
		friend class any;

	public:

		/** Returns type info for type `T` from database \a db. */
		template<typename T>
		[[nodiscard]] static type_info get(const type_database &db) noexcept { return {detail::type_data::bind<T>, db}; }

	private:
		constexpr type_info(const detail::type_data *data) noexcept : m_data(data) {}
		constexpr type_info(detail::type_handle handle, const type_database &db) noexcept : m_data(handle(db)) {}

	public:
		/** Initializes an invalid type info. */
		constexpr type_info() noexcept = default;

		/** Checks if the type info references a valid type. */
		[[nodiscard]] constexpr bool valid() const noexcept { return m_data != nullptr; }

		/** Returns the name of the referenced type. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return m_data->name; }
		/** Returns the size of the referenced type.
		 * @note If `is_empty_v<T>` evaluates to `true` for referenced type `T`, returns `0` instead of `1`. */
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
		[[nodiscard]] constexpr type_info remove_pointer(const type_database &db) const noexcept { return {m_data->remove_pointer, db}; }
		/** Removes extent from the referenced type. */
		[[nodiscard]] constexpr type_info remove_extent(const type_database &db) const noexcept { return {m_data->remove_extent, db}; }
		/** Returns the extent of the referenced type. */
		[[nodiscard]] constexpr std::size_t extent() const noexcept { return m_data->extent; }
		/** Checks if the referenced type is an array type. */
		[[nodiscard]] constexpr bool is_array() const noexcept { return extent() > 0; }

		[[nodiscard]] constexpr bool operator!=(const type_info &other) const noexcept = default;
		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept = default;

	private:
		const detail::type_data *m_data = nullptr;
	};

	constexpr type_info any::type() const noexcept { return m_type; }
}

#ifdef REFLEX_HEADER_ONLY
#include "detail/type_db_impl.hpp"
#endif