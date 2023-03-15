/*
 * Created by switchblade on 2023-03-13.
 */

#pragma once

#include "fwd.hpp"

namespace reflex::detail
{
	enum type_flags
	{
		/* Used by any, property_data & argument_data */
		IS_CONST = 0x1,
		IS_LVALUE = 0x2,
		IS_RVALUE = 0x4,

		/* Used by any */
		IS_OWNED = 0x8,
		IS_LOCAL = 0x10,

		/* Used by type_info */
		IS_NULL = 0x20,
		IS_VOID = 0x40,
		IS_ENUM = 0x80,
		IS_CLASS = 0x100,
		IS_POINTER = 0x200,
		IS_SIGNED_INT = 0x400,
		IS_UNSIGNED_INT = 0x800,
		IS_ARITHMETIC = 0x1000,
	};

	constexpr type_flags &operator&=(type_flags &a, std::underlying_type_t<type_flags> b) noexcept { return a = static_cast<type_flags>(a & b); }
	constexpr type_flags &operator|=(type_flags &a, std::underlying_type_t<type_flags> b) noexcept { return a = static_cast<type_flags>(a | b); }
	constexpr type_flags &operator^=(type_flags &a, std::underlying_type_t<type_flags> b) noexcept { return a = static_cast<type_flags>(a ^ b); }

	struct type_dtor
	{
		template<typename T>
		[[nodiscard]] constexpr static type_dtor bind() noexcept
		{
			return {
					+[](void *ptr) { std::destroy_at(static_cast<T *>(ptr)); },
					+[](void *ptr) { delete static_cast<T *>(ptr); }
			};
		}

		/* Destroy the object as-if via placement delete operator. */
		void (*destroy_in_place)(void *);
		/* Destroy the object as-if via deallocating delete operator. */
		void (*destroy_delete)(void *);
	};

	using type_handle = const type_data *(*)(const type_database &) noexcept;

	struct type_data
	{
		template<typename T>
		[[nodiscard]] inline static const type_data *bind(const type_database &) noexcept;

		std::string_view name;
		type_flags flags = {};
		std::size_t size = 0;

		type_handle remove_pointer;
		type_handle remove_extent;
		std::size_t extent = 0;

		type_dtor dtor;
	};
}