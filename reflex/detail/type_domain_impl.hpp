/*
 * Created by switchblade on 2023-03-14.
 */

#pragma once

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
				if (m_writer == self && m_writer_ctr)
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
			/* Decrement writer counter. If reached 0, reset writer id & clear the busy flag. */
			if (m_writer_ctr-- == 1)
			{
				m_writer.store({}, std::memory_order_relaxed);
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
				if (m_writer == self && m_writer_ctr)
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
				/* If already locked in read mode, increment the read counter. */
				if (m_writer == std::thread::id{} && m_reader_ctr)
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
			/* Decrement reader counter. If reached 0, clear the busy flag. */
			if (m_reader_ctr-- == 1)
				m_busy_flag.clear();
			/* Notify waiting threads. */
			m_busy_flag.notify_all();
		}
		bool domain_lock::try_lock_shared()
		{
			if (m_busy_flag.test_and_set())
			{
				/* If already locked in read mode, increment the read counter. */
				if (m_writer == std::thread::id{} && m_reader_ctr)
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

	detail::type_data *type_domain::try_reflect(std::string_view name, detail::type_data (*factory)() noexcept)
	{
		std::shared_lock<detail::domain_lock> rl{m_lock};
		auto iter = m_types.find(name);
		if (iter == m_types.end()) [[unlikely]]
		{
			/* Unable to find existing entry, lock for writing & insert. */
			rl.unlock();
			std::lock_guard<detail::domain_lock> wl{m_lock};
			iter = m_types.try_emplace(name, std::make_pair(factory(), factory)).first;
		}
		return &iter->second.first;
	}
}