/*
 * Created by switchblade on 2023-03-11.
 */

#pragma once

#include <memory>

#include "info.hpp"

namespace reflex
{
	namespace detail
	{
		template<typename T>
		struct any_deleter_func : std::false_type {};
		template<>
		struct any_deleter_func<void(void *)> : std::true_type {};
		template<>
		struct any_deleter_func<void(const void *)> : std::true_type {};
		template<>
		struct any_deleter_func<void (*)(void *)> : std::true_type {};
		template<>
		struct any_deleter_func<void (*)(const void *)> : std::true_type {};
	}

	/** Type-erased generic object. */
	class any
	{
		friend class type_info;
		template<typename>
		friend constexpr detail::any_funcs_t detail::make_any_funcs() noexcept;

		struct alignas(alignof(std::uintptr_t)) storage_t
		{
			template<typename T, std::ptrdiff_t Off>
			[[nodiscard]] T *get() noexcept
			{
				if constexpr (Off < 0)
				{
					static_assert(sizeof(T) + sizeof(bytes) + Off <= sizeof(storage_t));
					return reinterpret_cast<T *>(bytes + sizeof(bytes) + Off);
				}
				else
				{
					static_assert(sizeof(T) + Off <= sizeof(storage_t));
					return reinterpret_cast<T *>(bytes + Off);
				}
			}
			template<typename T, std::ptrdiff_t Off>
			[[nodiscard]] const T *get() const noexcept
			{
				if constexpr (Off < 0)
				{
					static_assert(sizeof(T) + sizeof(bytes) + Off <= sizeof(storage_t));
					return reinterpret_cast<const T *>(bytes + sizeof(bytes) + Off);
				}
				else
				{
					static_assert(sizeof(T) + Off <= sizeof(storage_t));
					return reinterpret_cast<const T *>(bytes + Off);
				}
			}

			std::byte bytes[sizeof(std::uintptr_t) * 2] = {};
		};

		template<typename T, typename U = std::decay_t<T>>
		static constexpr auto valid_deleter = std::is_empty_v<U> || detail::any_deleter_func<U>::value;
		template<typename T>
		static constexpr auto is_by_value = alignof(T) <= alignof(storage_t) && sizeof(T) <= (sizeof(storage_t) - sizeof(detail::type_flags)) &&
		                                    std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;

		[[noreturn]] static REFLEX_PUBLIC void throw_bad_any_cast(type_info from_type, type_info to_type);
		[[noreturn]] static REFLEX_PUBLIC void throw_bad_any_copy(type_info type);

	public:
		/** Initializes an empty `any`. */
		constexpr any() noexcept = default;

		/** Copies value of the managed object of \a other.
		 * @throw bad_any_copy If the underlying type of \a other is not copy-constructible. */
		any(const any &other) : any(other.type(), std::in_place, other.cdata()) {}
		/** If types of `this` and \a other are the same and `this` is not a reference, copy-assigns the managed object.
		 * Otherwise, destroys `this` and copy-constructs the managed object from \a other.
		 * @throw bad_any_copy If the managed object cannot be copy-assigned and the underlying type of \a other is not copy-constructible. */
		any &operator=(const any &other)
		{
			if (this != &other) assign(other.type(), std::in_place, other.cdata());
			return *this;
		}

		constexpr any(any &&other) noexcept { swap(other); }
		constexpr any &operator=(any &&other) noexcept
		{
			if (this != &other) swap(other);
			return *this;
		}

		~any() { destroy(); }

		/** Initializes `any` to manage a reference to \a ref. */
		template<typename T>
		explicit any(T &ref) requires (!std::same_as<std::decay_t<T>, any>) : any(type_of(ref), ref) {}
		/** Initializes `any` to manage a reference to \a ref using type info \a type. */
		template<typename T>
		any(type_info type, T &ref) requires (!std::same_as<std::decay_t<T>, any>)
		{
			this->type(type);
			if constexpr (std::is_const_v<T>)
				flags(detail::is_const);
			external(std::addressof(ref));
		}

		/** Initializes `any` to manage an instance of \a T move-constructed from \a value. */
		template<typename T>
		explicit any(T &&value) requires (!std::same_as<std::decay_t<T>, any>) : any(std::in_place_type<std::decay_t<T>>, std::forward<T>(value)) {}
		/** Initializes `any` to manage an instance of \a T move-constructed from \a value using type info \a type. */
		template<typename T>
		any(type_info type, T &&value) requires (!std::same_as<std::decay_t<T>, any>) : any(type, std::in_place_type<std::decay_t<T>>, std::forward<T>(value)) {}

		/** Initializes `any` to manage in-place constructed instance of \a T with arguments \a args. */
		template<typename T, typename... Args>
		explicit any(std::in_place_type_t<T>, Args &&...args) requires std::constructible_from<T, Args...> : any(type_info::get<T>(), std::in_place_type<T>, std::forward<Args>(args)...) {}
		/** Initializes `any` to manage in-place constructed instance of \a T with arguments \a args using type info \a type. */
		template<typename T, typename... Args>
		any(type_info type, std::in_place_type_t<T>, Args &&...args) requires std::constructible_from<T, Args...> { init_owned<T>(type, std::forward<Args>(args)...); }

		/** Initializes `any` to take ownership of \a ptr with deleter \a Del.
		 * @note Deleter must be an empty functor, a `void(void *)` or a `void(const void *)` function pointer. */
		template<typename T, typename Del = std::default_delete<T>>
		any(std::in_place_t, T *ptr, Del &&del = Del{}) requires(valid_deleter<Del>) : any(type_of(*ptr), std::in_place, ptr, std::forward<Del>(del)) {}
		/** Initializes `any` to take ownership of \a ptr with deleter \a Del using type info \a type.
		 * @note Deleter must be an empty functor, a `void(void *)` or a `void(const void *)` function pointer. */
		template<typename T, typename Del = std::default_delete<T>>
		any(type_info type, std::in_place_t, T *ptr, Del &&del = Del{}) requires valid_deleter<Del>
		{
			this->type(type);
			flags(detail::is_owned | (std::is_const_v<T> ? detail::is_const : detail::type_flags{}));
			if constexpr (!(std::is_function_v<Del> && std::is_invocable_v<Del, void *>))
				deleter(+[](void *ptr) { Del{}(detail::void_cast<T>(ptr)); });
			else
				deleter(del);
			external(ptr);
		}

		/** Initializes `any` to manage a reference to object of type \a type located at \a ptr. */
		any(type_info type, void *ptr) noexcept
		{
			this->type(type);
			external(ptr);
		}
		/** @copydoc any */
		any(type_info type, const void *ptr) noexcept
		{
			this->type(type);
			flags(detail::is_const);
			external(ptr);
		}

		/** Initializes `any` to manage an object of type \a type copy-constructed from value at \a ptr. */
		any(type_info type, std::in_place_t, void *ptr) { copy_init(type, ptr); }
		/** @copydoc any */
		any(type_info type, std::in_place_t, const void *ptr) { copy_init(type, ptr); }

		/** Initializes `any` to take ownership of \a ptr with deleter \a del using type info \a type. */
		any(type_info type, std::in_place_t, void *ptr, void (*del)(void *)) noexcept
		{
			this->type(type);
			flags(detail::is_owned);
			deleter(del);
			external(ptr);
		}
		/** @copydoc any */
		any(type_info type, std::in_place_t, const void *ptr, void (*del)(const void *)) noexcept
		{
			this->type(type);
			flags(detail::is_owned | detail::is_const);
			deleter(reinterpret_cast<void (*)(void *)>(del));
			external(ptr);
		}

		/** Replaces the managed object with a reference to \a ref. */
		template<typename T>
		any &operator=(T &ref) requires (!std::same_as<std::remove_cv_t<T>, any>) { return assign(type_of(ref), ref); }
		/** @copydoc operator= */
		template<typename T>
		any &assign(T &ref) requires (!std::same_as<std::remove_cv_t<T>, any>) { return operator=(ref); }
		/** Replaces the managed object with an in-place constructed instance of \a T with arguments \a args using type info \a type. */
		template<typename T>
		any &assign(type_info type, T &ref) requires (!std::same_as<std::remove_cv_t<T>, any>)
		{
			destroy();
			this->type(type);
			if constexpr (std::is_const_v<T>)
				flags(detail::is_const);
			external(std::addressof(ref));
			return *this;
		}

		/** Replaces the managed object with an in-place constructed instance of \a T with arguments \a args. */
		template<typename T, typename... Args>
		any &assign(std::in_place_type_t<T>, Args &&...args) requires std::constructible_from<T, Args...>
		{
			return assign(type_info::get<T>(), std::in_place_type<T>, std::forward<Args>(args)...);
		}
		/** Replaces the managed object with an in-place constructed instance of \a T with arguments \a args using type info \a type. */
		template<typename T, typename... Args>
		any &assign(type_info type, std::in_place_type_t<T>, Args &&...args) requires std::constructible_from<T, Args...>
		{
			destroy();
			init_owned<T>(type, std::forward<Args>(args)...);
			return *this;
		}

		/** Replaces the managed object with an copy-constructed instance of type \a type from value at \a ptr. */
		any &assign(type_info type, std::in_place_t, void *ptr)
		{
			copy_assign(type, ptr);
			return *this;
		}
		/** @copydoc assign */
		any &assign(type_info type, std::in_place_t, const void *ptr)
		{
			copy_assign(type, ptr);
			return *this;
		}

		/** Returns type of the managed object. */
		[[nodiscard]] type_info type() const noexcept { return {type_data(), m_db}; }

		/** Checks if the `any` has a managed object (either value or reference). */
		[[nodiscard]] bool empty() const noexcept { return type_data() == nullptr; }
		/** Checks if the managed object is const-qualified. */
		[[nodiscard]] bool is_const() const noexcept { return flags() & detail::is_const; }
		/** Checks if the `any` contains a reference to an external object. */
		[[nodiscard]] bool is_ref() const noexcept { return !(flags() & detail::is_owned); }

		/** Returns a void pointer to the managed object.
		 * @note If the managed object is const-qualified, returns `nullptr`. */
		[[nodiscard]] void *data() noexcept
		{
			if (flags() & detail::is_const) [[unlikely]] return nullptr;
			return (flags() & detail::is_value) ? local() : external();
		}
		/** Returns a const void pointer to the managed object. */
		[[nodiscard]] const void *cdata() const noexcept
		{
			return (flags() & detail::is_value) ? local() : external();
		}
		/** @copydoc cdata */
		[[nodiscard]] const void *data() const noexcept { return cdata(); }

		/** Resets `any` to an empty state. */
		void reset()
		{
			destroy();
			m_storage = {};
		}

		/** Returns an `any` containing a reference to the managed object of `this`. */
		[[nodiscard]] any ref() noexcept { return is_const() ? any{type(), cdata()} : any{type(), data()}; }
		/** Returns an `any` containing a constant reference to the managed object of `this`. */
		[[nodiscard]] any cref() const noexcept { return any{type(), cdata()}; }
		/** @copydoc cref */
		[[nodiscard]] any ref() const noexcept { return cref(); }

		/** @brief Casts the managed object to \a T.
		 * If the managed object can be represented with `T &`, equivalent to `static_cast<T &>(obj)`; otherwise,
		 * if the managed object is convertible to `T`, equivalent to `static_cast<T>(obj)`, where `obj` is the object
		 * managed by `this`.
		 *
		 * @return `any` containing the type-cast reference or value.
		 * @throw bad_any_cast If the managed object cannot be represented via `T &` and no conversions to `T` are available. */
		template<typename T>
		[[nodiscard]] any cast() { return cast(type_info::get<T>()); }
		/** @copydoc cast */
		template<typename T>
		[[nodiscard]] any cast() const { return cast(type_info::get<T>()); }
		/** @brief Casts the managed object to \a type.
		 * If the managed object can be represented with `T &`, equivalent to `static_cast<T &>(obj)`; otherwise,
		 * if the managed object is convertible to `T`, equivalent to `static_cast<T>(obj)`, where `obj` is the object
		 * managed by `this`, and `T` is the underlying type of \a type with same qualifiers as `obj`.
		 *
		 * @return `any` containing the type-cast reference or value.
		 * @throw bad_any_cast If the managed object cannot be represented via `T &` and no conversions to `T` are available. */
		[[nodiscard]] any cast(type_info type)
		{
			if (auto result = try_cast(type); result.empty())
				[[unlikely]] throw_bad_any_cast(this->type(), type);
			else
				return result;
		}
		/** @copydoc cast */
		[[nodiscard]] any cast(type_info type) const
		{
			if (auto result = try_cast(type); result.empty())
				[[unlikely]] throw_bad_any_cast(this->type(), type);
			else
				return result;
		}

		/** @brief Attempts to cast the managed object to \a T.
		 * If the managed object can be represented with `T &`, equivalent to `static_cast<T &>(obj)`; otherwise,
		 * if the managed object is convertible to `T`, equivalent to `static_cast<T>(obj)`, where `obj` is the object
		 * managed by `this`.
		 *
		 * @return `any` containing the type-cast reference or value, or an empty `any` if the
		 * managed object cannot be represented via `T &` and no conversions to `T` are available. */
		template<typename T>
		[[nodiscard]] any try_cast() { return try_cast(type_info::get<T>()); }
		/** @copydoc try_cast */
		template<typename T>
		[[nodiscard]] any try_cast() const { return try_cast(type_info::get<T>()); }
		/** @brief Attempts to cast the managed object to \a type.
		 * If the managed object can be represented with `T &`, equivalent to `static_cast<T &>(obj)`; otherwise,
		 * if the managed object is convertible to `T`, equivalent to `static_cast<T>(obj)`, where `obj` is the object
		 * managed by `this`, and `T` is the underlying type of \a type with same qualifiers as `obj`.
		 *
		 * @return `any` containing the type-cast reference or value, or an empty `any` if the
		 * managed object cannot be represented via `T &` and no conversions to `T` are available. */
		[[nodiscard]] REFLEX_PUBLIC any try_cast(type_info type);
		/** @copydoc try_cast */
		[[nodiscard]] REFLEX_PUBLIC any try_cast(type_info type) const;

		/** Returns pointer to the managed object or `nullptr` if the managed object is of a different type or const-ness than \a T.
		 * @tparam T Destination pointer type. */
		template<typename T>
		[[nodiscard]] T *try_get() noexcept
		{
			if constexpr (std::is_const_v<T>)
				return std::as_const(*this).try_get<T>();
			else if (type().name() == type_name_v<std::decay_t<T>>)
				return static_cast<T *>(data());
			else
				return nullptr;
		}
		/** Returns const pointer to the managed object or `nullptr` if the managed object is of a different type than \a T.
		 * @tparam T Destination pointer type. */
		template<typename T>
		[[nodiscard]] std::add_const_t<T> *try_get() const noexcept
		{
			if (type().name() == type_name_v<std::decay_t<T>>)
				return static_cast<std::add_const_t<T> *>(data());
			else
				return nullptr;
		}

		/** Returns reference to the managed object.
		 * @tparam T Destination reference type.
		 * @throw bad_any_cast If the managed object is of a different type or const-ness than \a T, or if the `any` is empty. */
		template<typename T>
		[[nodiscard]] T &get()
		{
			if (auto *ptr = try_get<T>(); ptr == nullptr)
				[[unlikely]] throw_bad_any_cast(type(), type_info::get<T>());
			else
				return *ptr;
		}
		/** Returns const reference to the managed object.
		 * @tparam T Destination reference type.
		 * @throw bad_any_cast If the managed object is of a different type than \a T, or if the `any` is empty. */
		template<typename T>
		[[nodiscard]] std::add_const_t<T> &get() const
		{
			if (auto *ptr = try_get<T>(); ptr == nullptr)
				[[unlikely]] throw_bad_any_cast(type(), type_info::get<T>());
			else
				return *ptr;
		}

		/** Returns pointer to the managed object, cast to \a T or `nullptr` if the managed object is of a different const-ness or not representable with \a T.
		 * @tparam T Destination pointer type. */
		template<typename T>
		[[nodiscard]] T *try_as()
		{
			if constexpr (std::is_const_v<T>)
				return std::as_const(*this).try_as<T>();
			else if (type().name() != type_name_v<std::decay_t<T>>)
				return static_cast<T *>(base_cast(type_name_v<std::decay_t<T>>));
			else
				return static_cast<T *>(data());
		}
		/** Returns const pointer to the managed object, cast to \a T or `nullptr` if the managed object is of a different const-ness or not representable with \a T.
		 * @tparam T Destination pointer type. */
		template<typename T>
		[[nodiscard]] std::add_const_t<T> *try_as() const
		{
			if (type().name() != type_name_v<std::decay_t<T>>)
				return static_cast<std::add_const_t<T> *>(base_cast(type_name_v<std::decay_t<T>>));
			else
				return static_cast<std::add_const_t<T> *>(data());
		}

		/** Returns reference to the managed object, cast to \a T.
		 * @tparam T Destination reference type.
		 * @throw bad_any_cast If the managed object is of a different const-ness or not representable with \a T, or if the `any` is empty. */
		template<typename T>
		[[nodiscard]] T &as()
		{
			if (auto *ptr = try_as<T>(); ptr == nullptr)
				[[unlikely]] throw_bad_any_cast(type(), type_info::get<T>());
			else
				return *ptr;
		}
		/** Returns const reference to the managed object, cast to \a T.
		 * @tparam T Destination reference type.
		 * @throw bad_any_cast If the managed object is not representable with \a T, or if the `any` is empty. */
		template<typename T>
		[[nodiscard]] std::add_const_t<T> &as() const
		{
			if (auto *ptr = try_as<T>(); ptr == nullptr)
				[[unlikely]] throw_bad_any_cast(type(), type_info::get<T>());
			else
				return *ptr;
		}

		/** Returns facet \a F for the managed object. */
		template<typename F>
		[[nodiscard]] inline F facet();
		/** @copydoc facet */
		template<typename F>
		[[nodiscard]] inline F facet() const;

		/** Swaps contents of `this` and `other`. */
		constexpr void swap(any &other) noexcept
		{
			swap_type(other);
			std::swap(m_storage, other.m_storage);
		}

		/** If the managed object of `this` is equal to the managed object of \a other, or if `this` and \a other are empty,
		 * returns `true`. Otherwise returns `false`. */
		[[nodiscard]] REFLEX_PUBLIC bool operator==(const any &other) const;
		/** If the managed object of `this` is not equal to the managed object of \a other, returns `true`. Otherwise returns `false`.  */
		[[nodiscard]] REFLEX_PUBLIC bool operator!=(const any &other) const;
		/** If the managed object of `this` is greater than or equal to the managed object of \a other, or if `this` is not empty while
		 * \a other is empty, or if both `this` and \a other are empty, returns `true`. Otherwise returns `false`.  */
		[[nodiscard]] REFLEX_PUBLIC bool operator>=(const any &other) const;
		/** If the managed object of `this` is less than or equal to the managed object of \a other, or if \a other is not empty while
		 * `this` is empty, or if both `this` and \a other are empty, returns `true`. Otherwise returns `false`.  */
		[[nodiscard]] REFLEX_PUBLIC bool operator<=(const any &other) const;
		/** If the managed object of `this` is greater than the managed object of \a other, or if `this` is not empty while
		 * \a other is empty, returns `true`. Otherwise returns `false`.  */
		[[nodiscard]] REFLEX_PUBLIC bool operator>(const any &other) const;
		/** If the managed object of `this` is less than the managed object of \a other, or if \a other is not empty while
		 * `this` is empty, returns `true`. Otherwise returns `false`.  */
		[[nodiscard]] REFLEX_PUBLIC bool operator<(const any &other) const;

	private:
		type_info type(type_info value) noexcept
		{
			const auto old_data = type_data(value.m_data);
			const auto old_db = std::exchange(m_db, value.m_db);
			return {old_data, old_db};
		}

		[[nodiscard]] void *local() noexcept { return m_storage.bytes; }
		[[nodiscard]] const void *local() const noexcept { return m_storage.bytes; }

		template<typename T = void(void *)>
		T *deleter(T *ptr) noexcept { return std::exchange(*m_storage.get<T *, 0>(), ptr); }
		template<typename T = void(void *)>
		[[nodiscard]] T *deleter() const noexcept { return *m_storage.get<T *, 0>(); }
		template<typename T = void>
		T *external(T *ptr) noexcept { return std::exchange(*m_storage.get<T *, sizeof(std::uintptr_t)>(), ptr); }
		template<typename T = void>
		[[nodiscard]] T *external() const noexcept { return *m_storage.get<T *, sizeof(std::uintptr_t)>(); }

		template<typename T, typename... Args>
		void init_owned(type_info type, Args &&...args)
		{
			this->type(type);
			auto flags = detail::is_owned | (std::is_const_v<T> ? detail::is_const : detail::type_flags{});
			if constexpr (!is_by_value < T >)
			{
				deleter(+[](void *ptr) { delete static_cast<std::remove_cv_t<T> *>(ptr); });
				external(new std::remove_cv_t<T>(std::forward<Args>(args)...));
			}
			else
			{
				std::construct_at(std::launder(static_cast<T *>(local())), std::forward<Args>(args)...);
				flags |= detail::is_value;
			}
			this->flags(flags);
		}
		template<typename T>
		inline void copy_init(type_info type, T *ptr);
		template<typename T>
		inline void copy_assign(type_info type, T *ptr);
		REFLEX_PUBLIC void destroy();

		template<typename T, typename U>
		void impl_copy_from(type_info type, U *data)
		{
			using other_t = take_const_t<T, U>;
			if constexpr (std::is_copy_constructible_v<T>)
				init_owned<T>(type, *static_cast<other_t *>(data));
			else
				throw_bad_any_copy(type);
		}
		template<typename T>
		void copy_from(type_info type, const void *cdata, void *data)
		{
			if (cdata != nullptr)
				impl_copy_from<T>(type, cdata);
			else
				impl_copy_from<T>(type, data);
		}
		template<typename T>
		void assign_from(type_info type, const void *cdata, void *data)
		{
			/* Attempt a copy-assignment for cases where `target` is by-value and assignable from source. */
			if constexpr (std::is_copy_assignable_v<T>)
				if (auto *tgt = try_get<T>(); !is_ref() && tgt != nullptr)
				{
					if (cdata != nullptr)
						*tgt = *static_cast<std::add_const_t<T> *>(cdata);
					else
						*tgt = *detail::void_cast<T>(data);
					return;
				}
			copy_from<T>(type, cdata, data);
		}

		[[nodiscard]] REFLEX_PUBLIC void *base_cast(std::string_view) const;
		[[nodiscard]] REFLEX_PUBLIC any value_conv(std::string_view) const;

#ifdef NDEBUG
		/* Flags are stored within the type data pointer. */
		const detail::type_data *type_data(const detail::type_data *ptr) noexcept
		{
			const auto old_flags = static_cast<std::uintptr_t>(m_data_ptr_flags & detail::any_flags_max);
			m_data_ptr_flags = std::bit_cast<std::uintptr_t>(ptr) | old_flags;
			return ptr;
		}
		[[nodiscard]] const detail::type_data *type_data() const noexcept
		{
			return std::bit_cast<const detail::type_data *>(m_data_ptr_flags & ~static_cast<std::uintptr_t>(detail::any_flags_max));
		}

		detail::type_flags flags(detail::type_flags value) noexcept
		{
			const auto old_intptr = (m_data_ptr_flags & ~static_cast<std::uintptr_t>(detail::any_flags_max));
			m_data_ptr_flags = old_intptr | static_cast<std::uintptr_t>(value);
			return value;
		}
		[[nodiscard]] detail::type_flags flags() const noexcept
		{
			return static_cast<detail::type_flags>(m_data_ptr_flags & detail::any_flags_max);
		}

		constexpr void swap_type(any &other) noexcept
		{
			std::swap(m_data_ptr_flags, other.m_data_ptr_flags);
			std::swap(m_db, other.m_db);
		}

		std::uintptr_t m_data_ptr_flags = 0;
#else
		const detail::type_data *type_data(const detail::type_data *ptr) noexcept { return m_type = ptr; }
		[[nodiscard]] const detail::type_data *type_data() const noexcept { return m_type; }

		detail::type_flags flags(detail::type_flags value) noexcept { return m_flags = value; }
		[[nodiscard]] detail::type_flags flags() const noexcept { return m_flags; }

		constexpr void swap_type(any &other) noexcept
		{
			std::swap(m_type, other.m_type);
			std::swap(m_flags, other.m_flags);
			std::swap(m_db, other.m_db);
		}

		const detail::type_data *m_type = nullptr;
		detail::type_flags m_flags = {};
#endif

		detail::database_impl *m_db = nullptr;
		storage_t m_storage;
	};

	constexpr void swap(any &a, any &b) noexcept { a.swap(b); }

	/** Returns the type info of the object managed by \a value. Equivalent to `value.type()`. */
	template<typename T>
	[[nodiscard]] inline type_info type_of(T &&value) requires std::same_as<std::decay_t<T>, any> { return value.type(); }

	/** Returns an `any` referencing the object managed by \a other. */
	template<typename T>
	[[nodiscard]] inline any forward_any(T &other) requires(std::same_as<std::decay_t<T>, any>) { return other.ref(); }
	/** Returns an `any` referencing the object at \a instance. */
	template<typename T>
	[[nodiscard]] inline any forward_any(T &instance) requires (!std::same_as<std::decay_t<T>, any>) { return any{instance}; }

	/** Returns an `any` containing a move-constructed instance of \a value. */
	template<typename T>
	[[nodiscard]] inline any forward_any(T &&value) { return any{std::forward<std::decay_t<T>>(value)}; }
	/** Forwards the value of \a other. */
	[[nodiscard]] inline any forward_any(any &&other) { return any{std::forward<any>(other)}; }

	/** Returns an `any` owning an instance of \a T constructed with arguments \a args. */
	template<typename T, typename... Args>
	[[nodiscard]] inline any make_any(Args &&...args) { return any{std::in_place_type<T>, std::forward<Args>(args)...}; }

	template<typename T>
	any type_info::attribute() const { return attribute(type_name_v<std::decay_t<T>>); }
	any type_info::attribute(type_info type) const { return attribute(type.name()); }

	template<typename T>
	bool type_info::has_enumeration(T &&value) const requires (!std::same_as<std::decay_t<T>, any>) { return has_enumeration(forward_any(std::forward<T>(value))); }

	template<std::size_t N>
	any type_info::construct(std::span<any, N> args) const { return construct(std::span<any>{args}); }
	template<typename... Args>
	any type_info::construct(Args &&... args) const
	{
		if constexpr (sizeof...(Args) == 0)
			return construct(std::span<any>{});
		else
		{
			auto args_array = std::array{forward_any(args)...};
			return construct(std::span<any>{args_array});
		}
	}
}