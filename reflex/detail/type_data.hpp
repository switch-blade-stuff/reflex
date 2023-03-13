/*
 * Created by switchblade on 2023-03-13.
 */

#pragma once

#include "fwd.hpp"

namespace reflex::detail
{
	enum type_flags : std::size_t
	{
		IS_CONST = 0x1,
		IS_VOLATILE = 0x2,

		IS_LVALUE = 0x4,
		IS_RVALUE = 0x8,
		IS_POINTER = 0x10,

		IS_ENUM = 0x20,
		IS_CLASS = 0x40,
	};

	[[nodiscard]] constexpr type_flags operator&(type_flags a, std::size_t b) noexcept
	{
		return static_cast<type_flags>(static_cast<std::size_t>(a) & b);
	}
	[[nodiscard]] constexpr type_flags operator|(type_flags a, std::size_t b) noexcept
	{
		return static_cast<type_flags>(static_cast<std::size_t>(a) | b);
	}
	[[nodiscard]] constexpr type_flags operator^(type_flags a, std::size_t b) noexcept
	{
		return static_cast<type_flags>(static_cast<std::size_t>(a) ^ b);
	}

	constexpr type_flags &operator&=(type_flags &a, std::size_t b) noexcept { return a = a & b; }
	constexpr type_flags &operator|=(type_flags &a, std::size_t b) noexcept { return a = a | b; }
	constexpr type_flags &operator^=(type_flags &a, std::size_t b) noexcept { return a = a ^ b; }

	struct type_handle
	{
		template<typename T>
		[[nodiscard]] constexpr static type_handle bind() noexcept;

		[[nodiscard]] constexpr const type_data *operator->() const { return get(); }
		const type_data *(*get)() = +[]() -> const type_data * { return nullptr; };
	};

	struct type_dtor
	{
		template<typename T>
		[[nodiscard]] constexpr static type_dtor bind() noexcept
		{
			return
			{
				.destroy_in_place = +[](void *ptr) { std::destroy_at(static_cast<T *>(ptr)); },
				.destroy_delete = +[](void *ptr) { delete static_cast<T *>(ptr); }
			};
		}

		/* Destroy the object as-if via placement delete operator. */
		void (*destroy_in_place)(void *);
		/* Destroy the object as-if via deallocating delete operator. */
		void (*destroy_delete)(void *);
	};

	struct type_data
	{
		template<typename T>
		[[nodiscard]] constexpr static type_data bind() noexcept;

		std::string_view name;
		type_flags flags = {};
		std::size_t size;

		type_handle add_const;
		type_handle remove_const;
		type_handle add_volatile;
		type_handle remove_volatile;
		type_handle add_cv;
		type_handle remove_cv;

		type_handle add_rvalue;
		type_handle add_lvalue;
		type_handle remove_ref;
		type_handle add_pointer;
		type_handle remove_pointer;

		std::size_t extent = 0;
		type_handle remove_extent;
		type_handle decay;

		type_dtor dtor;
	};

	template<typename T>
	constexpr type_handle type_handle::bind() noexcept
	{
		return +[]()
		{
			constexpr auto data = type_data::bind<T>();
			// TODO: Reflect data with the primary database.
			// static const auto data_ptr = type_database::primary().reflect(&data);
			static const auto data_ptr = &data;
			return data_ptr;
		};
	}

	template<typename T>
	constexpr type_data type_data::bind() noexcept
	{
		type_data result;

		result.name = type_name_v<T>;
		result.size = std::is_empty_v<T> ? 0 : sizeof(T);

		if constexpr (std::is_const_v<T>) result.flags |= type_flags::IS_CONST;
		if constexpr (std::is_volatile_v<T>) result.flags |= type_flags::IS_VOLATILE;

		result.add_const = type_handle::bind<std::add_const_t<T>>();
		result.remove_const = type_handle::bind<std::remove_const_t<T>>();
		result.add_volatile = type_handle::bind<std::add_volatile_t<T>>();
		result.remove_volatile = type_handle::bind<std::remove_volatile_t<T>>();
		result.add_cv = type_handle::bind<std::add_cv_t<T>>();
		result.remove_cv = type_handle::bind<std::remove_cvref_t<T>>();

		if constexpr (std::is_rvalue_reference_v<T>) result.flags |= type_flags::IS_RVALUE;
		if constexpr (std::is_lvalue_reference_v<T>) result.flags |= type_flags::IS_LVALUE;
		if constexpr (std::is_pointer_v<T>) result.flags |= type_flags::IS_POINTER;

		result.add_rvalue = type_handle::bind<std::add_rvalue_reference_t<T>>();
		result.add_lvalue = type_handle::bind<std::add_lvalue_reference_t<T>>();
		result.remove_ref = type_handle::bind<std::remove_reference_t<T>>();
		result.add_pointer = type_handle::bind<std::add_pointer_t<T>>();
		result.remove_pointer = type_handle::bind<std::remove_pointer_t<T>>();

		if constexpr (std::is_enum_v<T>) result.flags |= type_flags::IS_ENUM;
		if constexpr (std::is_class_v<T>) result.flags |= type_flags::IS_CLASS;

		result.extent = std::extent_v<T>;
		result.remove_extent = type_handle::bind<std::remove_extent_t<T>>();
		result.decay = type_handle::bind<std::decay_t<T>>();

		/* Constructors & destructors are only created for object types. */
		if constexpr (std::is_object_v<T>)
		{
			result.dtor = type_dtor::bind<T>();
		}

		return result;
	}
}