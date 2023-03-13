/*
 * Created by switchblade on 2023-03-13.
 */

#pragma once

#include "detail/facet.hpp"

namespace reflex
{
	class type_info
	{
		friend class any;

	public:

		/** Returns type info for type `T`. */
		template<typename T>
		[[nodiscard]] static type_info get() noexcept { return detail::type_handle::bind<T>(); }

	private:
		constexpr type_info(const detail::type_data *data) noexcept : m_data(data) {}
		constexpr type_info(detail::type_handle handle) noexcept : m_data(handle.get()) {}

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
		/** Returns the extent of the referenced type. */
		[[nodiscard]] constexpr std::size_t extent() const noexcept { return m_data->extent; }

		/** Checks if the referenced type is empty. */
		[[nodiscard]] constexpr bool is_empty() const noexcept { return size() == 0; }

		/** Checks if the referenced type is const-qualified. */
		[[nodiscard]] constexpr bool is_const() const noexcept { return m_data->flags & detail::IS_CONST; }
		/** Checks if the referenced type is volatile-qualified. */
		[[nodiscard]] constexpr bool is_volatile() const noexcept { return m_data->flags & detail::IS_VOLATILE; }
		/** Checks if the referenced type is const- and volatile-qualified. */
		[[nodiscard]] constexpr bool is_cv() const noexcept { return m_data->flags & (detail::IS_CONST | detail::IS_VOLATILE); }

		/** Adds `const` qualifier to the referenced type. */
		[[nodiscard]] constexpr type_info add_const() const noexcept { return m_data->add_const; }
		/** Removes `const` qualifier from the referenced type. */
		[[nodiscard]] constexpr type_info remove_const() const noexcept { return m_data->remove_const; }
		/** Adds `volatile` qualifier to the referenced type. */
		[[nodiscard]] constexpr type_info add_volatile() const noexcept { return m_data->add_volatile; }
		/** Removes `volatile` qualifier from the referenced type. */
		[[nodiscard]] constexpr type_info remove_volatile() const noexcept { return m_data->remove_volatile; }
		/** Adds `const` and `volatile` qualifiers to the referenced type. */
		[[nodiscard]] constexpr type_info add_cv() const noexcept { return m_data->add_cv; }
		/** Removes `const` and `volatile` qualifiers from the referenced type. */
		[[nodiscard]] constexpr type_info remove_cv() const noexcept { return m_data->remove_cv; }

		/** Checks if the referenced type is an lvalue reference. */
		[[nodiscard]] constexpr bool is_lvalue_reference() const noexcept { return m_data->flags & detail::IS_LVALUE; }
		/** Checks if the referenced type is an rvalue reference. */
		[[nodiscard]] constexpr bool is_rvalue_reference() const noexcept { return m_data->flags & detail::IS_RVALUE; }
		/** Checks if the referenced type is a reference. */
		[[nodiscard]] constexpr bool is_reference() const noexcept { return m_data->flags & (detail::IS_LVALUE | detail::IS_RVALUE); }
		/** Checks if the referenced type is a pointer type.
		 * @note For pointer-like class types, check the `pointer_like` facet. */
		[[nodiscard]] constexpr bool is_pointer() const noexcept { return m_data->flags & detail::IS_POINTER; }

		/** Adds pointer to the referenced type. */
		[[nodiscard]] constexpr type_info add_pointer() const noexcept { return m_data->add_pointer; }
		/** Removes pointer from the referenced type. */
		[[nodiscard]] constexpr type_info remove_pointer() const noexcept { return m_data->remove_pointer; }
		/** Adds an lvalue reference to the referenced type. */
		[[nodiscard]] constexpr type_info add_lvalue_reference() const noexcept { return m_data->add_lvalue; }
		/** Adds an rvalue reference to the referenced type. */
		[[nodiscard]] constexpr type_info add_rvalue_reference() const noexcept { return m_data->add_rvalue; }
		/** Removes reference from the referenced type. */
		[[nodiscard]] constexpr type_info remove_lvalue_reference() const noexcept { return m_data->remove_ref; }

		/** Checks if the referenced type is an enum type. */
		[[nodiscard]] constexpr bool is_enum() const noexcept { return m_data->flags & detail::IS_ENUM; }
		/** Checks if the referenced type is a class type. */
		[[nodiscard]] constexpr bool is_class() const noexcept { return m_data->flags & detail::IS_CLASS; }

		/** Checks if the referenced type is an array type. */
		[[nodiscard]] constexpr bool is_array() const noexcept { return extent() > 0; }
		/** Removes extent from the referenced type. */
		[[nodiscard]] constexpr type_info remove_extent() const noexcept { return m_data->remove_extent; }

		/** Decays the referenced type. */
		[[nodiscard]] constexpr type_info decay() const noexcept { return m_data->decay; }

		[[nodiscard]] constexpr bool operator!=(const type_info &other) const noexcept = default;
		[[nodiscard]] constexpr bool operator==(const type_info &other) const noexcept = default;

	private:
		const detail::type_data *m_data = nullptr;
	};

	constexpr type_info any::type() const noexcept { return m_type; }
}