/*
 * Created by switchblade on 2023-03-14.
 */

#pragma once

#include <cassert>

#include "database.hpp"

namespace reflex::detail
{
	shared_spinlock &shared_spinlock::lock()
	{
		/* Wait until not busy. */
		while (m_busy_flag.test_and_set())
			m_busy_flag.wait(true);
		return *this;
	}
	shared_spinlock &shared_spinlock::lock_shared()
	{
		while (m_busy_flag.test_and_set())
		{
			/* If already locked in read mode, increment the read counter. */
			if (m_reader_ctr != 0)
			{
				m_reader_ctr++;
				return *this;
			}
			/* Otherwise wait until not busy. */
			m_busy_flag.wait(true);
		}
		/* Busy flag was not set previously, acquire the lock. */
		m_reader_ctr++;
		m_busy_flag.notify_all();
		return *this;
	}

	void shared_spinlock::unlock()
	{
		assert(m_reader_ctr == 0);

		m_busy_flag.clear();
		m_busy_flag.notify_all();
	}
	void shared_spinlock::unlock_shared()
	{
		assert(m_reader_ctr != 0);

		/* Decrement reader counter. If reached 0, clear the busy flag. */
		if (m_reader_ctr-- == 1)
			m_busy_flag.clear();
		/* Notify waiting threads. */
		m_busy_flag.notify_all();
	}

	struct spinlock_guard
	{
		static spinlock_guard make_unique(shared_spinlock &l) { return spinlock_guard{.m_lock = l.lock(), .m_locked_unique = true}; }
		static spinlock_guard make_shared(shared_spinlock &l) { return spinlock_guard{.m_lock = l.lock_shared(), .m_locked_shared = true}; }

		~spinlock_guard() { unlock(); }

		void relock(bool unique = true)
		{
			if (unique && !m_locked_unique)
			{
				unlock();
				m_lock.lock();
				m_locked_unique = true;
			}
			else if (!unique && !m_locked_shared)
			{
				unlock();
				m_lock.lock_shared();
				m_locked_shared = true;
			}
		}
		void unlock()
		{
			if (std::exchange(m_locked_shared, false))
				m_lock.unlock_shared();
			else if (std::exchange(m_locked_unique, false))
				m_lock.unlock();
		}

		shared_spinlock &m_lock;
		bool m_locked_unique = false;
		bool m_locked_shared = false;
	};

	database_impl *database_impl::instance() noexcept { return global_ptr(); }
	database_impl *database_impl::instance(database_impl *ptr) noexcept { return global_ptr().exchange(ptr); }

	database_impl::database_impl() noexcept = default;
	database_impl::~database_impl() = default;

	void database_impl::reset()
	{
		const auto g = spinlock_guard::make_unique(*this);
		for (auto &[_, entry]: m_types) entry.first = entry.second(*this);
	}
	const type_data *database_impl::reset(std::string_view name)
	{
		const auto g = spinlock_guard::make_unique(*this);
		const auto iter = m_types.find(name);
		if (iter == m_types.end()) return nullptr;

		iter->second.first = iter->second.second(*this);
		return &iter->second.first;
	}
	const type_data *database_impl::find(std::string_view name) const
	{
		const auto g = spinlock_guard::make_shared(const_cast<database_impl &>(*this));
		if (const auto iter = m_types.find(name); iter != m_types.end())
			return &iter->second.first;
		else
			return nullptr;
	}

	type_data *database_impl::reflect(std::string_view name, type_data (*factory)(database_impl &))
	{
		auto g = spinlock_guard::make_shared(*this);
		auto iter = m_types.find(name);
		if (iter == m_types.end()) [[unlikely]]
		{
			/* Unable to find existing entry, lock for writing & insert. */
			g.relock();
			iter = m_types.try_emplace(name, std::make_pair(factory(*this), factory)).first;
		}
		return &iter->second.first;
	}
}