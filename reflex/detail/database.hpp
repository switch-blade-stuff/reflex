/*
 * Created by switchblade on 2023-03-14.
 */

#pragma once

#include "factory.hpp"

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

			REFLEX_PUBLIC type_data *reflect(std::string_view name, type_data (*factory)(database_impl &));

			tpp::stable_map<std::string_view, std::pair<type_data, type_data (*)(database_impl &)>> m_types;
		};

		template<typename T>
		[[nodiscard]] inline static type_data *make_type_data(database_impl &db)
		{
			static type_data *data = db.reflect(type_name<T>::value, +[](database_impl &db) -> type_data
			{
				auto result = type_data{type_name_v<T>};
				if constexpr (std::same_as<T, void>)
					result.flags |= type_flags::IS_VOID;
				else
				{
					result.alignment = alignof(T);
					if constexpr (!std::is_empty_v<T>) result.size = sizeof(T);

					if constexpr (std::is_null_pointer_v<T>) result.flags |= type_flags::IS_NULL;

					if constexpr (std::is_class_v<T>) result.flags |= type_flags::IS_CLASS;
					if constexpr (std::is_pointer_v<T>) result.flags |= type_flags::IS_POINTER;
					if constexpr (std::is_abstract_v<T>) result.flags |= type_flags::IS_ABSTRACT;

					result.remove_pointer = make_type_data<std::decay_t<std::remove_pointer_t<T>>>;
					result.remove_extent = make_type_data<std::decay_t<std::remove_extent_t<T>>>;
					result.extent = std::extent_v<T>;
				}

				/* Constructors, destructors & conversions are only created for object types. */
				if constexpr (std::is_object_v<T>)
				{
					if constexpr (std::is_enum_v<T>)
					{
						result.flags |= type_flags::IS_ENUM;
						result.conv_list.emplace(type_name_v<std::underlying_type_t<T>>, make_type_conv<T, std::underlying_type_t<T>>());
					}
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

					/* If `T` is derived from `object`, add `object` to the list of parent types. */
					if constexpr (std::derived_from<T, object> && !std::same_as<T, object>)
						result.base_list.emplace(type_name_v<object>, make_type_base<T, object>());

					/* Add default & copy constructors. */
					if constexpr (std::is_default_constructible_v<T>)
						result.default_ctor = &result.ctor_list.emplace_back(make_type_ctor<T>());
					if constexpr (std::is_copy_constructible_v<T>)
						result.copy_ctor = &result.ctor_list.emplace_back(make_type_ctor<T, std::add_const_t<T> &>());

					result.dtor = [](void *ptr) { std::destroy_at(static_cast<T *>(ptr)); };
					result.any_funcs = make_any_funcs<T>();
				}

				/* Invoke user type initializer. */
				if constexpr (requires(type_factory<T> f) { type_init<T>{}(f); })
					type_init<T>{}(type_factory<T>{&result, &db});

				return result;
			});
			return data;
		}
	}

	template<typename T>
	type_factory<T> type_info::reflect()
	{
		auto *db = detail::database_impl::instance();
		return {detail::make_type_data<std::decay_t<T>>, *db};
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
		return {detail::make_type_data<std::decay_t<T>>, *db};
	}

	void type_info::reset(std::string_view name) { detail::database_impl::instance()->reset(name); }
	template<typename T>
	void type_info::reset() { reset(type_name_v<std::decay_t<T>>); }
	void type_info::reset() { detail::database_impl::instance()->reset(); }
}
