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
			REFLEX_PUBLIC type_data *reset(std::string_view name);
			REFLEX_PUBLIC type_data *insert(const constant_type_data &data);
			REFLEX_PUBLIC const type_data *find(std::string_view name) const;

			tpp::stable_map<std::string_view, type_data> m_types;
		};

		template<typename T>
		type_data *make_type_data(database_impl &db)
		{
			static type_data *data = db.insert(make_constant_type_data<T>());
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
