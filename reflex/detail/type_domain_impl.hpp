/*
 * Created by switchblade on 2023-03-14.
 */

#pragma once

#include <cassert>

#include "type_domain.hpp"
#include "type_info.hpp"

namespace reflex
{
	namespace detail
	{
		void domain_lock::lock()
		{
			const auto self = std::this_thread::get_id();
			while (m_busy_flag.test_and_set())
			{
				/* If the lock is owned by us, increment the writer counter. */
				if (is_writing(self))
				{
					m_writer_ctr++;
					return;
				}
				/* Otherwise, wait until not busy and try again. */
				m_busy_flag.wait(false);
			}
			/* Busy flag was not set previously, acquire the lock. */
			acquire_write(self);
		}
		void domain_lock::unlock()
		{
			assert(m_writer_ctr != 0);
			assert(m_writer == std::this_thread::get_id());

			/* Decrement writer counter. If the writer counter reaches 0, reset the writer thread id.
			 * If both counters reach 0, clear the busy flag as well. */
			if (m_writer_ctr-- == 1)
			{
				m_writer.store({}, std::memory_order_relaxed);
				if (m_reader_ctr == 0)
					m_busy_flag.clear(std::memory_order_relaxed);
				std::atomic_thread_fence(std::memory_order_release);
			}
			/* Notify waiting threads. */
			m_busy_flag.notify_all();
		}
		bool domain_lock::try_lock()
		{
			const auto self = std::this_thread::get_id();
			if (m_busy_flag.test_and_set())
			{
				/* If we are the writing thread, increment write counter. */
				if (is_writing(self))
				{
					m_writer_ctr++;
					return true;
				}
				return false;
			}
			/* Flag was not set previously, acquire the lock. */
			acquire_write(self);
			return true;
		}

		void domain_lock::lock_shared()
		{
			while (m_busy_flag.test_and_set())
			{
				/* If already locked in read mode, or the current thread is the writer, increment the read counter. */
				if (is_reading() || is_writing(std::this_thread::get_id()))
				{
					m_reader_ctr++;
					return;
				}
				/* Otherwise wait until not busy. */
				m_busy_flag.wait(false);
			}
			/* Busy flag was not set previously, acquire the lock. */
			acquire_read();
		}
		void domain_lock::unlock_shared()
		{
			assert(m_reader_ctr != 0);

			/* Decrement reader counter. If both counters reach 0, clear the busy flag. */
			if (m_reader_ctr-- == 1 && !m_writer_ctr)
				m_busy_flag.clear();
			/* Notify waiting threads. */
			m_busy_flag.notify_all();
		}
		bool domain_lock::try_lock_shared()
		{
			if (m_busy_flag.test_and_set())
			{
				/* If already locked in read mode, or the current thread is the writer, increment the read counter. */
				if (is_reading() || is_writing(std::this_thread::get_id()))
				{
					m_reader_ctr++;
					return true;
				}
				return false;
			}
			/* Flag was not set previously, acquire the lock. */
			acquire_read();
			return true;
		}
	}

	type_domain *type_domain::instance(type_domain *ptr) { return guard_base::instance(ptr); }
	typename type_domain::guard_type type_domain::instance() { return guard_base::instance(); }

	template<typename T>
	type_info type_domain::reset() { return reset(type_name_v<T>); }
	type_info type_domain::reset(std::string_view name)
	{
		const auto iter = m_types.find(name);
		if (iter == m_types.end()) return type_info{};

		iter->second.first = iter->second.second();
		return type_info{&iter->second.first, this};
	}

	template<typename T>
	type_info type_domain::find() const { return find(type_name_v<T>); }
	type_info type_domain::find(std::string_view name) const
	{
		/* Const-casting here is fine. Domain must already be mutable for m_types to not be empty. */
		if (const auto iter = m_types.find(name); iter != m_types.end())
			return {&iter->second.first, const_cast<type_domain *>(this)};
		return {};
	}

	detail::type_data *type_domain::try_reflect(std::string_view name, data_factory factory)
	{
		struct lock_guard
		{
			lock_guard(detail::domain_lock &l) : m_lock(l) { lock_shared(); }
			~lock_guard() { unlock(); }

			void lock_shared()
			{
				m_lock.lock_shared();
				m_locked_shared = true;
			}
			void relock_unique()
			{
				unlock();
				m_lock.lock();
				m_locked_unique = true;
			}
			void unlock()
			{
				if (std::exchange(m_locked_shared, false))
					m_lock.unlock_shared();
				else if (std::exchange(m_locked_unique, false))
					m_lock.unlock();
			}

			detail::domain_lock &m_lock;
			bool m_locked_unique = false;
			bool m_locked_shared = false;
		};

		lock_guard g{m_lock};
		auto iter = m_types.find(name);
		if (iter == m_types.end()) [[unlikely]]
		{
			/* Unable to find existing entry, lock for writing & insert. */
			g.relock_unique();
			iter = m_types.try_emplace(name, std::make_pair(factory(), factory)).first;
		}
		return &iter->second.first;
	}
}