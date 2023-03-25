/*
 * Created by switchblade on 2023-03-14.
 */

#pragma once

#include "type_data.hpp"

namespace reflex
{
	namespace detail
	{
		/* Use a shared spinlock to synchronize database operations, since thread contention is expected to be low. */
		class shared_spinlock
		{
#ifndef REFLEX_NO_THREADS
		public:
			REFLEX_PUBLIC shared_spinlock &lock();
			REFLEX_PUBLIC shared_spinlock &lock_shared();

			REFLEX_PUBLIC void unlock();
			REFLEX_PUBLIC void unlock_shared();

		private:
			std::atomic_flag m_busy_flag = {};
			std::atomic<std::thread::id> m_writer = {};
			std::atomic<std::size_t> m_reader_ctr = {};
#else
			public:
				void lock() {}
				void unlock() {}
				bool try_lock() { return true; }

				void lock_shared() {}
				void unlock_shared() {}
				bool try_lock_shared() { return true; }
#endif
		};

		struct database_impl : shared_spinlock
		{
			using data_factory = type_data (*)() noexcept;

			static database_impl *local_ptr() noexcept
			{
				static database_impl value;
				return &value;
			}
			static std::atomic<database_impl *> &global_ptr() noexcept
			{
				static std::atomic<database_impl *> value = local_ptr();
				return value;
			}

			[[nodiscard]] REFLEX_PUBLIC static database_impl *instance() noexcept;
			REFLEX_PUBLIC static database_impl *instance(database_impl *ptr) noexcept;

			REFLEX_PUBLIC database_impl() noexcept;
			REFLEX_PUBLIC ~database_impl();

			REFLEX_PUBLIC void reset();
			REFLEX_PUBLIC const type_data *reset(std::string_view name);
			REFLEX_PUBLIC const type_data *find(std::string_view name) const;

			REFLEX_PUBLIC type_data *reflect(std::string_view name, data_factory factory);

			tpp::stable_map<std::string_view, std::pair<type_data, data_factory>> m_types;
		};

		template<typename T>
		[[nodiscard]] inline static type_data *make_type_data(database_impl &db)
		{
			/* Cache type_data instance to avoid needless lookups. `database_impl::reflect` does its own synchronization,
			 * so it is fine to not initialize the pointer directly. */
			constinit static type_data *data = nullptr;
			if (data == nullptr) [[unlikely]]
			{
				data = db.reflect(type_name<T>::value, +[]() noexcept -> type_data
				{
					auto result = type_data{type_name_v<T>};
					if constexpr (std::same_as<T, void>)
						result.flags |= type_flags::IS_VOID;
					else
					{
						if constexpr (!std::is_empty_v<T>) result.size = sizeof(T);

						if constexpr (std::is_null_pointer_v<T>) result.flags |= type_flags::IS_NULL;
						if constexpr (std::is_enum_v<T>) result.flags |= type_flags::IS_ENUM;
						if constexpr (std::is_class_v<T>) result.flags |= type_flags::IS_CLASS;
						if constexpr (std::is_pointer_v<T>) result.flags |= type_flags::IS_POINTER;

						result.remove_pointer = make_type_data<std::remove_pointer_t<T>>;
						result.remove_extent = make_type_data<std::remove_extent_t<T>>;
						result.extent = std::extent_v<T>;
					}

					/* Constructors, destructors & conversions are only created for object types. */
					if constexpr (std::is_object_v<T>)
					{
						if constexpr (std::convertible_to<T, std::intmax_t>)
						{
							result.flags |= type_flags::IS_SIGNED_INT;
							result.conv_list.emplace(type_name_v<std::intmax_t>, make_type_conv<T, std::intmax_t>());
						}
						if constexpr (std::convertible_to<T, std::uintmax_t>)
						{
							result.flags |= type_flags::IS_UNSIGNED_INT;
							result.conv_list.emplace(type_name_v<std::uintmax_t>, make_type_conv<T, std::uintmax_t>());
						}
						if constexpr (std::convertible_to<T, double>)
						{
							result.flags |= type_flags::IS_ARITHMETIC;
							result.conv_list.emplace(type_name_v<double>, make_type_conv<T, double>());
						}

						/* Add default & copy constructors. */
						if constexpr (std::is_default_constructible_v<T>)
							result.default_ctor = &result.ctor_list.emplace_back(make_type_ctor<T>());
						if constexpr (std::is_copy_constructible_v<T>)
							result.copy_ctor = &result.ctor_list.emplace_back(make_type_ctor<T, std::add_const_t<T> &>());

						result.dtor = make_type_dtor<T>();
						result.any_funcs = make_any_funcs<T>();
					}

					return result;
				});
			}
			return data;
		}
	}

	type_info type_info::get(std::string_view name)
	{
		auto *db = detail::database_impl::instance();
		return {db->find(name), db};
	}
	template<typename T>
	type_info type_info::get()
	{
		auto *db = detail::database_impl::instance();
		return {detail::make_type_data<std::remove_cvref_t<T>>, *db};
	}
}
