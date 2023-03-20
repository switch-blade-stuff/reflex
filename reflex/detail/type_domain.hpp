/*
 * Created by switchblade on 2023-03-14.
 */

#pragma once

#define TPP_STL_HASH_ALL
#include <tpp/stable_map.hpp>
#include <tpp/stl_hash.hpp>

#include "../access_guard.hpp"
#include "type_data.hpp"

namespace reflex
{
	namespace detail
	{
		class domain_lock
		{
#ifndef REFLEX_NO_THREADS
		public:
			REFLEX_PUBLIC void lock();
			REFLEX_PUBLIC void unlock();
			REFLEX_PUBLIC bool try_lock();

			REFLEX_PUBLIC void lock_shared();
			REFLEX_PUBLIC void unlock_shared();
			REFLEX_PUBLIC bool try_lock_shared();

		private:
			void acquire_read()
			{
				m_reader_ctr++;
				/* Notify waiting threads to move forward with read locks. */
				m_busy_flag.notify_all();
			}
			void acquire_write(std::thread::id writer)
			{
				m_writer.store(writer, std::memory_order_relaxed);
				m_writer_ctr.fetch_add(1, std::memory_order_relaxed);
				std::atomic_thread_fence(std::memory_order_release);
			}

			std::atomic_flag m_busy_flag = {};
			std::atomic<std::thread::id> m_writer = {};
			std::atomic<std::size_t> m_writer_ctr = {};
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

		struct domain_lock_base { mutable detail::domain_lock m_lock; };
	}

	/** Database of reflected type information. */
	class type_domain : guarded_instance<type_domain, detail::domain_lock>, detail::domain_lock_base
	{
		using guard_base = guarded_instance<type_domain, detail::domain_lock>;
		using guard_type = access_guard<type_domain *, detail::domain_lock>;

		friend struct detail::type_data;
		friend class argument_info;
		friend class type_info;
		friend guard_base;

		[[nodiscard]] static constexpr mutex_type &instance_mtx(auto *ptr) noexcept { return ptr->m_lock; }

	public:
		/** Returns access guard to the global type domain. */
		[[nodiscard]] REFLEX_PUBLIC static guard_type instance();
		/** Atomically replaces the global access guard instance with \a ptr and returns pointer to the old instance. */
		REFLEX_PUBLIC static type_domain *instance(type_domain *ptr);

	private:
		static constexpr guard_type guard(const type_domain *domain) { return {const_cast<type_domain *>(domain), domain->m_lock}; }

	public:
		constexpr type_domain() = default;

		type_domain(const type_domain &) = delete;
		type_domain(type_domain &&) = delete;
		type_domain &operator=(const type_domain &) = delete;
		type_domain &operator=(type_domain &&) = delete;

		/** Resets all reflected types to initial state. */
		[[nodiscard]] REFLEX_PUBLIC type_info reset();

		/** Resets type info for type \a T to initial state, and returns the new `type_info`.
		 * If the type has not reflected yet, returns an invalid `type_info`. */
		template<typename T>
		inline type_info reset();
		/** Resets type info for type with name \a name to initial state, and returns the new `type_info`.
		 * If the type has not reflected yet, returns an invalid `type_info`. */
		REFLEX_PUBLIC type_info reset(std::string_view name);

		/** Returns `type_info` for type \a T, or an invalid `type_info` if the type was not reflected yet. */
		template<typename T>
		[[nodiscard]] inline type_info find() const;
		/** Returns `type_info` for type with name \a name, or an invalid `type_info` if the type was not reflected yet. */
		[[nodiscard]] REFLEX_PUBLIC type_info find(std::string_view name) const;

	private:
		[[nodiscard]] REFLEX_PUBLIC detail::type_data *try_reflect(std::string_view name, detail::type_data (*factory)() noexcept);

		tpp::stable_map<std::string_view, std::pair<detail::type_data, detail::type_data (*)() noexcept>> m_types;
	};
}
