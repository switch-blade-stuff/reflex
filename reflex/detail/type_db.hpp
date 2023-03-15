/*
 * Created by switchblade on 2023-03-14.
 */

#pragma once

#define TPP_STL_HASH_ALL
#include <tpp/sparse_map.hpp>
#include <tpp/stl_hash.hpp>

#include "../access_guard.hpp"
#include "type_data.hpp"

namespace reflex
{
	/** Database of reflected type information. */
	class type_database : shared_guarded_instance<type_database>
	{
		friend struct detail::type_data;

	public:
		/** Returns access guard to the global type database. */
		[[nodiscard]] REFLEX_PUBLIC static shared_guard<type_database *> instance();
		/** Atomically replaces the global access guard instance with \a ptr and returns pointer to the old instance. */
		REFLEX_PUBLIC static type_database *instance(type_database *ptr);

	public:

	private:
		[[nodiscard]] REFLEX_PUBLIC const detail::type_data *data_or(std::string_view name, const detail::type_data &init) const;

		tpp::sparse_map<std::string_view, detail::type_data> m_types;
	};

	template<typename T>
	const detail::type_data *detail::type_data::bind(const type_database &db) noexcept
	{
		constexpr auto data = []()
		{
			type_data result;
			result.name = type_name_v<T>;

			if constexpr (std::same_as<T, void>)
				result.flags |= type_flags::IS_VOID;
			else
			{
				if constexpr (!std::is_empty_v<T>) result.size = sizeof(T);

				if constexpr (std::is_null_pointer_v<T>) result.flags |= type_flags::IS_NULL;
				if constexpr (std::is_enum_v<T>) result.flags |= type_flags::IS_ENUM;
				if constexpr (std::is_class_v<T>) result.flags |= type_flags::IS_CLASS;
				if constexpr (std::is_pointer_v<T>) result.flags |= type_flags::IS_POINTER;

				if constexpr (std::signed_integral<T> || std::convertible_to<T, std::intmax_t>)
				{
					result.flags |= type_flags::IS_SIGNED_INT;
					/* TODO: Add std::intmax_t conversion. */
				}
				if constexpr (std::unsigned_integral<T> || std::convertible_to<T, std::uintmax_t>)
				{
					result.flags |= type_flags::IS_UNSIGNED_INT;
					/* TODO: Add std::uintmax_t conversion. */
				}
				if constexpr (std::is_arithmetic_v<T> || std::convertible_to<T, double>)
				{
					result.flags |= type_flags::IS_ARITHMETIC;
					/* TODO: Add double conversion. */
				}

				result.remove_pointer = bind<std::remove_pointer_t<T>>;
				result.remove_extent = bind<std::remove_extent_t<T>>;
				result.extent = std::extent_v<T>;

				/* Constructors & destructors are only created for object types. */
				if constexpr (std::is_object_v<T>)
				{
					result.dtor = type_dtor::bind<T>();
				}
			}
			return result;
		}();
		return db.data_or(type_name<T>::value, data);
	}
}
