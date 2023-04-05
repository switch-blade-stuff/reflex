/*
 * Created by switchblade on 2023-04-01.
 */

#pragma once

#include "spinlock.hpp"

namespace reflex::detail
{
#ifndef REFLEX_NO_THREADS
	void shared_spinlock::lock()
	{
		auto expected = flags_t{};
		for (std::size_t i = 0; !m_flags.compare_exchange_weak(expected, is_writing); ++i)
			if (i >= spin_max) m_flags.wait(expected);
	}
	void shared_spinlock::unlock()
	{
		m_flags = flags_t{};
		m_flags.notify_all();
	}
	bool shared_spinlock::try_lock()
	{
		auto expected = flags_t{};
		return m_flags.compare_exchange_weak(expected, is_writing);
	}

	void shared_spinlock::lock_shared() const
	{
		auto expected = flags_t{};
		for (std::size_t i = 0; !m_flags.compare_exchange_weak(expected, is_reading); ++i)
		{
			/* Quit if the reader counter already reading. */
			if (expected == is_reading) break;
			if (i >= spin_max) m_flags.wait(expected);
		}
		m_reader_ctr++;
	}
	void shared_spinlock::unlock_shared() const
	{
		if (m_reader_ctr-- == 1)
		{
			m_flags = flags_t{};
			m_flags.notify_all();
		}
	}
	bool shared_spinlock::try_lock_shared() const
	{
		auto expected = flags_t{};
		if (!m_flags.compare_exchange_weak(expected, is_writing) && expected != is_reading)
			return false;

		m_reader_ctr++;
		return true;
	}
#endif
}