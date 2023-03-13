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

	struct type_dtor
	{
		/* Destroy the object as-if via placement delete operator. */
		void (*destroy_in_place)(void *);
		/* Destroy the object as-if via deallocating delete operator. */
		void (*destroy_delete)(void *);
	};

	struct type_data
	{
		std::string_view name;
		std::size_t extent = 0;
		std::size_t size;
		type_flags flags;

		const type_data *add_const = nullptr;
		const type_data *remove_const = nullptr;
		const type_data *add_volatile = nullptr;
		const type_data *remove_volatile = nullptr;
		const type_data *add_cv = nullptr;
		const type_data *remove_cv = nullptr;

		const type_data *add_rvalue = nullptr;
		const type_data *add_lvalue = nullptr;
		const type_data *remove_ref = nullptr;
		const type_data *add_pointer = nullptr;
		const type_data *remove_pointer = nullptr;
		const type_data *remove_extent = nullptr;

		const type_data *decay = nullptr;

		type_dtor dtor;
	};
}