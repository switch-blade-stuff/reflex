/*
 * Created by switchblade on 17/06/22
 */

#pragma once

#ifndef REFLEX_NO_THREADS

#include <concepts>
#include <optional>
#include <utility>
#include <mutex>

#include <shared_mutex>

#endif

namespace reflex
{
	namespace detail
	{
		template<typename T>
		concept basic_lockable =
		requires(T &value)
		{
			value.lock();
			value.unlock();
		};
		template<typename T>
		concept lockable =
		requires(T &value)
		{
			requires basic_lockable<T>;
			{ value.try_lock() } -> std::same_as<bool>;
		};
		template<typename T>
		concept shared_lockable =
		requires(T &value)
		{
			requires lockable<T>;
			value.lock_shared();
			value.unlock_shared();
			{ value.try_lock_shared() } -> std::same_as<bool>;
		};

#ifdef REFLEX_NO_THREADS
		/* Define dummy mutex types. */
		struct mutex
		{
			constexpr void lock() const noexcept {}
			constexpr void unlock() const noexcept {}
			constexpr bool try_lock() const noexcept { return true; }
		};
		struct shared_mutex : mutex
		{
			constexpr void lock_shared() const noexcept {}
			constexpr void unlock_shared() const noexcept {}
			constexpr bool try_lock_shared() const noexcept { return true; }
		};
		using recursive_mutex = mutex;
#else
		using mutex = std::mutex;
		using shared_mutex = std::shared_mutex;
		using recursive_mutex = std::recursive_mutex;
#endif
	}

	template<typename P, detail::basic_lockable Mutex = detail::mutex, bool = detail::shared_lockable<Mutex>>
	class access_guard;
	template<typename T, detail::basic_lockable Mutex = detail::mutex>
	class guarded_instance;

	/** Pointer-like accessor returned by `access_guard`. */
	template<typename P, typename L>
	class access_handle
	{
		template<typename, detail::basic_lockable, bool>
		friend
		class access_guard;

		using traits_t = std::pointer_traits<P>;

	public:
		typedef typename traits_t::element_type element_type;

	public:
		access_handle() = delete;
		access_handle(const access_handle &) = delete;
		access_handle &operator=(const access_handle &) = delete;

#ifdef REFLEX_NO_THREADS
		constexpr access_handle(access_handle &&other) noexcept(std::is_nothrow_move_constructible_v<P>) = default;
		constexpr access_handle &operator=(access_handle &&other) noexcept(std::is_nothrow_move_assignable_v<P>) = default;

		/** Initializes an access handle for pointer-like type \a ptr and lock \a lock. */
		constexpr access_handle(const P &ptr, [[maybe_unused]] L &&lock) : access_handle(ptr) {}
		/** @copydoc access_handle */
		constexpr access_handle(P &&ptr, [[maybe_unused]] L &&lock) : access_handle(std::move(ptr)) {}

		/** Initializes an access handle for pointer-like type \a ptr. */
		constexpr access_handle(const P &ptr) : m_ptr(ptr) {}
		/** @copydoc access_handle */
		constexpr access_handle(P &&ptr) : m_ptr(std::move(ptr)) {}
#else
		constexpr access_handle(access_handle &&other) noexcept(std::is_nothrow_move_constructible_v<P> && std::is_nothrow_move_constructible_v<L>) = default;
		constexpr access_handle &operator=(access_handle &&other) noexcept(std::is_nothrow_move_assignable_v<P> && std::is_nothrow_move_assignable_v<L>) = default;

		/** Initializes an access handle for pointer-like type \a ptr and lock \a lock. */
		constexpr access_handle(const P &ptr, L &&lock) : m_ptr(ptr), m_lock(std::move(lock)) {}
		/** @copydoc access_handle */
		constexpr access_handle(P &&ptr, L &&lock) : m_ptr(std::move(ptr)), m_lock(std::move(lock)) {}
#endif

		/** Returns the stored pointer as if via `std::to_address`. */
		[[nodiscard]] constexpr decltype(auto) get() const noexcept { return std::to_address(m_ptr); }
		/** @copydoc get */
		[[nodiscard]] constexpr decltype(auto) operator->() const noexcept { return get(); }
		/** Dereferences the guarded pointer. */
		[[nodiscard]] constexpr decltype(auto) operator*() const noexcept { return *m_ptr; }

		[[nodiscard]] constexpr auto operator<=>(const access_handle &other) const requires std::three_way_comparable<P> { return m_ptr <=> other.m_ptr; }
		[[nodiscard]] constexpr bool operator==(const access_handle &other) const requires std::equality_comparable<P> { return m_ptr == other.m_ptr; }

	private:
		P m_ptr;
#ifndef REFLEX_NO_THREADS
		L m_lock;
#endif
	};

	/** Smart pointer used to provide synchronized access to an instance of a type.
	 * @tparam P Pointer-like type guarded by the access guard.
	 * @tparam Mutex Type of mutex used to synchronize \a P. */
	template<typename P, detail::basic_lockable Mutex, bool>
	class access_guard
	{
		using traits_t = std::pointer_traits<P>;

#ifdef REFLEX_NO_THREADS
		/* Define dummy lock. */
		struct unique_lock
		{
			constexpr unique_lock() noexcept = default;
			constexpr unique_lock(Mutex &) noexcept {}
		};
#else
		typedef std::unique_lock<Mutex> unique_lock;
#endif

	public:
		typedef Mutex mutex_type;
		typedef typename traits_t::element_type element_type;

		typedef access_handle<P, unique_lock> unique_handle;

	public:
		/** Initializes an empty access guard. */
		constexpr access_guard() noexcept = default;

#ifdef REFLEX_NO_THREADS
		/** Initializes an access guard for a pointer of type `P`. */
		constexpr access_guard(const P &ptr) noexcept : m_ptr(ptr) {}
		/** @copydoc access_guard */
		constexpr access_guard(P &&ptr) noexcept : m_ptr(std::move(ptr)) {}

		/** Initializes an access guard for a pointer of type `P` and mutex instance. */
		constexpr access_guard(const P &ptr, mutex_type &) noexcept : access_guard(ptr) {}
		/** @copydoc access_guard */
		constexpr access_guard(P &&ptr, mutex_type &) noexcept : access_guard(std::move(ptr)) {}
#else
		/** Initializes an access guard for a pointer of type `P` and mutex instance. */
		constexpr access_guard(const P &ptr, mutex_type &mtx) noexcept : m_ptr(ptr), m_mtx(&mtx) {}
		/** @copydoc access_guard */
		constexpr access_guard(P &&ptr, mutex_type &mtx) noexcept : m_ptr(std::move(ptr)), m_mtx(&mtx) {}
#endif

		/** Checks if the access guard is empty (does not point to any object). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_ptr == nullptr; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Acquires a unique lock and returns an accessor handle. */
		[[nodiscard]] constexpr unique_handle access() &{ return {m_ptr, unique_lock{*mutex()}}; }
		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle get() &{ return access(); }
		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle operator->() &{ return access(); }

		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle access() &&{ return {std::move(m_ptr), unique_lock{*mutex()}}; }
		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle get() &&{ return access(); }
		/** @copydoc access */
		[[nodiscard]] constexpr unique_handle operator->() &&{ return access(); }

		/** Attempts to acquire a unique lock and returns an optional accessor handle. */
		[[nodiscard]] constexpr std::optional<unique_handle> try_access() &requires detail::lockable<Mutex>
		{
			if (mutex()->try_lock()) [[likely]]
				return {unique_handle{m_ptr, unique_lock{*mutex(), std::adopt_lock}}};
			else
				return {std::nullopt};
		}
		/** @copydoc try_access */
		[[nodiscard]] constexpr std::optional<unique_handle> try_access() &&requires detail::lockable<Mutex>
		{
			if (mutex()->try_lock()) [[likely]]
				return {unique_handle{std::move(m_ptr), unique_lock{*mutex(), std::adopt_lock}}};
			else
				return {std::nullopt};
		}

		/** Returns the underlying pointer. */
		[[nodiscard]] constexpr P &pointer() noexcept { return m_ptr; }
		/** @copydoc value */
		[[nodiscard]] constexpr const P &pointer() const noexcept { return m_ptr; }

#ifdef REFLEX_NO_THREADS
		/** Returns `nullptr`. */
		[[nodiscard]] constexpr mutex_type *mutex() noexcept { return nullptr; }
		/** @copydoc mutex */
		[[nodiscard]] constexpr const mutex_type *mutex() const noexcept { return nullptr; }
#else
		/** Returns pointer to the underlying mutex. */
		[[nodiscard]] constexpr mutex_type *mutex() noexcept { return m_mtx; }
		/** @copydoc mutex */
		[[nodiscard]] constexpr const mutex_type *mutex() const noexcept { return m_mtx; }
#endif

		[[nodiscard]] constexpr auto operator<=>(const access_guard &other) const requires std::three_way_comparable<P> { return m_ptr <=> other.m_ptr; }
		[[nodiscard]] constexpr bool operator==(const access_guard &other) const requires std::equality_comparable<P> { return m_ptr == other.m_ptr; }

	protected:
		P m_ptr = {};
#ifndef REFLEX_NO_THREADS
		mutex_type *m_mtx = {};
#endif
	};
	/** Overload of `access_guard` for shared mutex types. */
	template<typename P, detail::basic_lockable Mutex>
	class access_guard<P, Mutex, true> : public access_guard<P, Mutex, false>
	{
		using base_t = access_guard<P, Mutex, false>;
		using traits_t = std::pointer_traits<P>;

		using const_ptr = typename traits_t::template rebind<std::add_const_t<typename base_t::element_type>>;

#ifdef REFLEX_NO_THREADS
		struct shared_lock { constexpr shared_lock(Mutex &) noexcept = default; };
#else
		using shared_lock = std::shared_lock<Mutex>;
#endif

		using base_t::m_ptr;

	public:
		using shared_handle = access_handle<const_ptr, shared_lock>;

	public:
		using access_guard<P, Mutex, false>::operator=;
		using access_guard<P, Mutex, false>::operator->;
		using access_guard<P, Mutex, false>::access_guard;

		/** Acquires a shared lock and returns an accessor handle. */
		[[nodiscard]] constexpr shared_handle access_shared() &{ return {m_ptr, shared_lock{*this->mutex()}}; }
		/** @copydoc access_shared */
		[[nodiscard]] constexpr shared_handle access_shared() &&{ return {std::move(m_ptr), shared_lock{*this->mutex()}}; }
		/** @copydoc access_shared */
		[[nodiscard]] constexpr shared_handle access_shared() const &{ return {m_ptr, shared_lock{*this->mutex()}}; }
		/** @copydoc access_shared */
		[[nodiscard]] constexpr shared_handle access_shared() const &&{ return {std::move(m_ptr), shared_lock{*this->mutex()}}; }
		/** @copydoc access_shared */
		[[nodiscard]] constexpr shared_handle get() const &{ return access_shared(); }
		/** @copydoc access_shared */
		[[nodiscard]] constexpr shared_handle get() const &&{ return access_shared(); }
		/** @copydoc access_shared */
		[[nodiscard]] constexpr shared_handle operator->() const &{ return access_shared(); }
		/** @copydoc access_shared */
		[[nodiscard]] constexpr shared_handle operator->() const &&{ return access_shared(); }

		/** Attempts to acquire a unique lock and returns an optional accessor handle. */
		[[nodiscard]] constexpr std::optional<shared_handle> try_access_shared() const
		{
			if (base_t::m_mtx.try_lock_shared()) [[likely]]
				return {shared_handle{m_ptr, shared_lock{*this->mutex(), std::adopt_lock}}};
			else
				return {std::nullopt};
		}
	};

	template<typename P, typename M>
	access_guard(const P &, M &) -> access_guard<P, M>;
	template<typename P, typename M>
	access_guard(P &&, M &) -> access_guard<std::remove_reference_t<P>, M>;

	/** Type alias for `access_guard` that uses `std::recursive_mutex` as its mutex type. */
	template<typename P>
	using recursive_guard = access_guard<P, detail::recursive_mutex>;
	/** Type alias for `access_guard` that uses `std::shared_mutex` as its mutex type. */
	template<typename P>
	using shared_guard = access_guard<P, detail::shared_mutex>;

	/** @brief Structure used to implement a thread-safe guarded singleton.
	 *
	 * This structure provides a common implementation of guarded global instance functions.
	 * CRTP child type may implement the following functions:
	 * <ul>
	 * <li>`static pointer-like local_ptr()` - used to obtain object instance pointer (usually pointer to a local static variable)</li>
	 * <li>`static pointer-like &global_ptr()` - used to obtain object instance pointer (usually default-initialized from `local_ptr()`)</li>
	 * <li>`static Mutex &instance_mtx(pointer-like &)` - used to obtain reference to the instance mutex.</li>
	 * </ul>
	 * If any of the functions are not available, uses an internal implementation.
	 *
	 * @tparam T Singleton child type.
	 * @tparam Mutex Mutex type used by the global access guard. */
	template<typename T, detail::basic_lockable Mutex>
	class guarded_instance
	{
	public:
		using mutex_type = Mutex;

	private:
		[[nodiscard]] inline static auto invoke_local_ptr();
		[[nodiscard]] inline static auto &invoke_global_ptr();

#ifndef REFLEX_NO_THREADS
		[[nodiscard]] inline static Mutex &invoke_instance_mtx(auto &);

	public:
		/** Returns pointer to the global access guard. */
		[[nodiscard]] static auto instance()
		{
			auto &ptr = invoke_global_ptr();
			return access_guard{ptr, invoke_instance_mtx(ptr)};
		}
		/** Atomically exchanges the global access guard pointer with \a ptr. */
		static auto instance(const auto &ptr)
		{
			using ptr_type = std::remove_reference_t<decltype(invoke_global_ptr())>;
			if constexpr (requires { std::atomic<ptr_type>{invoke_global_ptr()}; })
				return std::atomic<ptr_type>{invoke_global_ptr()}.exchange(ptr);
			else
			{
				std::lock_guard<Mutex> l{invoke_instance_mtx()};
				return std::exchange(invoke_global_ptr(), ptr);
			}
		}
#else
		public:
			/** Returns pointer to the global access guard. */
			[[nodiscard]] static auto instance()
			{
				using ptr_type = std::remove_reference_t<decltype(invoke_global_ptr())>;
				return access_guard<ptr_type, Mutex>{invoke_global_ptr()};
			}
			/** Exchanges the global access guard pointer with \a ptr. */
			static auto instance(const auto &ptr) { return std::exchange(invoke_global_ptr(), ptr); }
#endif
	};

	template<typename T, detail::basic_lockable Mutex>
	auto &guarded_instance<T, Mutex>::invoke_global_ptr()
	{
		if constexpr (requires { T::global_ptr(); })
			return T::global_ptr();
		else
		{
			static T *ptr;
			return ptr;
		}
	}
	template<typename T, detail::basic_lockable Mutex>
	auto guarded_instance<T, Mutex>::invoke_local_ptr()
	{
		if constexpr (requires { T::local_ptr(); })
			return T::local_ptr();
		else
		{
			static T value;
			return &value;
		}
	}
#ifndef REFLEX_NO_THREADS
	template<typename T, detail::basic_lockable Mutex>
	Mutex &guarded_instance<T, Mutex>::invoke_instance_mtx(auto &ptr)
	{
		if constexpr (requires {{ T::instance_mtx(ptr) } -> std::same_as<Mutex &>; })
			return T::instance_mtx(ptr);
		else
		{
			static Mutex mtx;
			return mtx;
		}
	}
#endif

	/** Type alias for `guarded_instance` that uses `std::recursive_mutex` as its mutex type. */
	template<typename T>
	using recursive_guarded_instance = guarded_instance<T, detail::recursive_mutex>;
	/** Type alias for `guarded_instance` that uses `std::shared_mutex` as its mutex type. */
	template<typename T>
	using shared_guarded_instance = guarded_instance<T, detail::shared_mutex>;
}