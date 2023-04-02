/*
 * Created by switchblade on 2023-04-01.
 */

#pragma once

#include <atomic>
#include <tuple>

#include "define.hpp"

namespace reflex::detail
{
	/* Use a shared spinlock to synchronize type & database operations, since thread contention is expected to be low. */
	class shared_spinlock
	{
#ifndef REFLEX_NO_THREADS
		enum flags_t
		{
			IS_READING = 1,
			IS_WRITING = 2,
		};

		constexpr static std::size_t spin_max = 12;

	public:
		REFLEX_PUBLIC void lock();
		REFLEX_PUBLIC void unlock();
		REFLEX_PUBLIC bool try_lock();

		REFLEX_PUBLIC void lock_shared() const;
		REFLEX_PUBLIC void unlock_shared() const;
		REFLEX_PUBLIC bool try_lock_shared() const;

	private:
		void spin_condition(flags_t flags, auto &&f)
		{
			auto expected = flags_t{};
			for (std::size_t i = 0; !m_flags.compare_exchange_weak(expected, flags); ++i)
			{
				f(expected);
				if (i >= spin_max) m_flags.wait(expected);
			}
		}

		mutable std::atomic<flags_t> m_flags = {};
		mutable std::atomic<std::size_t> m_reader_ctr = {};
#else
	public:
		void lock() {}
		void unlock() {}
		bool try_lock() { return true; }

		void lock_shared() const {}
		void unlock_shared() const {}
		bool try_lock_shared() const { return true; }
#endif
	};

	/* Custom implementation of scoped_lock + shared_scoped_lock to avoid including mutex & shared_mutex */

	template<typename... Ts>
	inline void unlock(Ts &...locks) { (locks.unlock(), ...); }

	template<typename T, typename... Ts>
	inline bool next_lock(T &lock, Ts &...rest) noexcept
	{
		auto result = lock.try_lock();
		if constexpr (sizeof...(rest) != 0)
		{
			result &= next_lock(rest...);
			if (!result) lock.unlock();
		}
		return result;
	}
	template<typename T, typename... Ts>
	inline void lock(T &lock, Ts &...rest)
	{
		lock.lock();
		if constexpr (sizeof...(rest) != 0)
		{
			if (!next_lock(rest...))
				lock.unlock();
		}
	}

	template<typename... Ts>
	struct scoped_lock
	{
		scoped_lock(Ts &...ls) : locks{ls...} { lock(ls...); }
		~scoped_lock() { std::apply(unlock<Ts...>, locks); }

		std::tuple<Ts &...> locks;
	};

	template<typename... Ts>
	scoped_lock(Ts &...) -> scoped_lock<Ts...>;

	template<typename... Ts>
	inline void unlock_shared(Ts &...locks) { (locks.unlock_shared(), ...); }

	template<typename T, typename... Ts>
	inline bool next_lock_shared(T &lock, Ts &...rest) noexcept
	{
		auto result = lock.try_lock_shared();
		if constexpr (sizeof...(rest) != 0)
		{
			result &= next_lock_shared(rest...);
			if (!result) lock.unlock_shared();
		}
		return result;
	}
	template<typename T, typename... Ts>
	inline void lock_shared(T &lock, Ts &...rest)
	{
		lock.lock_shared();
		if constexpr (sizeof...(rest) != 0)
		{
			if (!next_lock_shared(rest...))
				lock.unlock_shared();
		}
	}

	template<typename... Ts>
	struct shared_scoped_lock
	{
		shared_scoped_lock(Ts &...ls) : locks{ls...} { lock_shared(ls...); }
		~shared_scoped_lock() { std::apply(unlock_shared<Ts...>, locks); }

		std::tuple<Ts &...> locks;
	};

	template<typename... Ts>
	shared_scoped_lock(Ts &...) -> shared_scoped_lock<Ts...>;
}