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
		static spinlock_guard make_unique(shared_spinlock &l) { return spinlock_guard{.m_unique = &l.lock()}; }
		static spinlock_guard make_shared(shared_spinlock &l) { return spinlock_guard{.m_shared = &l.lock_shared()}; }

		~spinlock_guard()
		{
			if (m_unique) m_unique->unlock();
			if (m_shared) m_shared->unlock_shared();
		}

		shared_spinlock *m_unique = nullptr;
		shared_spinlock *m_shared = nullptr;
	};

	database_impl *database_impl::instance() noexcept { return global_ptr(); }
	database_impl *database_impl::instance(database_impl *ptr) noexcept { return global_ptr().exchange(ptr); }

	database_impl::database_impl() noexcept = default;
	database_impl::~database_impl() = default;

	void database_impl::reset()
	{
		const auto g = spinlock_guard::make_unique(*this);
		for (auto &[_, entry]: m_types) entry.reset(*this);
	}
	type_data *database_impl::reset(std::string_view name)
	{
		const auto g = spinlock_guard::make_unique(*this);
		if (auto iter = m_types.find(name); iter != m_types.end())
			[[likely]] return &iter->second.reset(*this);
		else
			return nullptr;
	}
	type_data *database_impl::insert(const constant_type_data &data)
	{
		const auto g = spinlock_guard::make_unique(*this);
		return &m_types.emplace(data.name, data).first->second.init(*this);
	}

	const type_data *database_impl::find(std::string_view name) const
	{
		const auto g = spinlock_guard::make_shared(const_cast<database_impl &>(*this));
		if (const auto iter = m_types.find(name); iter != m_types.end())
			return &iter->second;
		else
			return nullptr;
	}
}