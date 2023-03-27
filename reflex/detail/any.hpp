/*
 * Created by switchblade on 2023-03-11.
 */

#pragma once

#include <memory>

#include "type_info.hpp"

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

		[[nodiscard]] inline static bad_argument_list make_ctor_error(type_info, std::span<any>);
		template<typename T, typename... Ts>
		[[nodiscard]] inline static bad_argument_list make_ctor_error();
	}

	/** Exception type thrown when the managed object of `any` cannot be cast to the desired type. */
	class bad_any_cast : public std::runtime_error
	{
		[[nodiscard]] static std::string make_msg(type_info from_type, type_info to_type)
		{
			std::string result;
			result.reserve(from_type.name().size() + to_type.name().size() + 78);
			result.append("Managed object of type `");
			result.append(from_type.name());
			result.append("` cannot be represented as or converted to type `");
			result.append(to_type.name());
			result.append(1, '`');
			return result;
		}

	public:
		bad_any_cast(const bad_any_cast &) = default;
		bad_any_cast &operator=(const bad_any_cast &) = default;
		bad_any_cast(bad_any_cast &&) = default;
		bad_any_cast &operator=(bad_any_cast &&) = default;

		/** Initializes the any cast exception from source type info and destination type info. */
		bad_any_cast(type_info from_type, type_info to_type) : std::runtime_error(make_msg(from_type, to_type)), m_from_type(from_type), m_to_type(to_type) {}

		REFLEX_PUBLIC ~bad_any_cast() override = default;

		/** Returns type info of the converted-from type. */
		[[nodiscard]] constexpr type_info from_type() const noexcept { return m_from_type; }
		/** Returns type info of the converted-to type. */
		[[nodiscard]] constexpr type_info to_type() const noexcept { return m_to_type; }

	private:
		type_info m_from_type;
		type_info m_to_type;
	};

	/** Type-erased generic object. */
	class any
	{
		friend class type_info;
		template<typename>
		friend detail::any_funcs_t detail::make_any_funcs() noexcept;

		struct alignas(std::max(alignof(std::uintptr_t), alignof(std::uintmax_t))) storage_t
		{
			template<typename T, std::ptrdiff_t Off>
			[[nodiscard]] T *get() noexcept
			{
				if constexpr (Off < 0)
					return reinterpret_cast<T *>(bytes + Off + sizeof(bytes));
				else
					return reinterpret_cast<T *>(bytes + Off);
			}
			template<typename T, std::ptrdiff_t Off>
			[[nodiscard]] const T *get() const noexcept
			{
				if constexpr (Off < 0)
					return reinterpret_cast<const T *>(bytes + Off + sizeof(bytes));
				else
					return reinterpret_cast<const T *>(bytes + Off);
			}

			std::byte bytes[std::max(sizeof(std::uintptr_t), sizeof(std::uintmax_t)) * 3] = {};
		};

		struct ptr_storage
		{
			void (*deleter)(void *) = nullptr;
			void *data = nullptr;
		};

		template<typename T, typename U = std::decay_t<T>>
		static constexpr auto valid_deleter = std::is_empty_v<U> || detail::any_deleter_func<U>::value;
		template<typename T>
		static constexpr auto is_by_value = alignof(T) <= alignof(storage_t) && sizeof(T) <= (sizeof(storage_t) - sizeof(detail::type_flags)) &&
		                                    std::is_trivially_move_constructible_v<T>;

	public:
		/** Initializes an empty `any`. */
		constexpr any() noexcept = default;

		constexpr any(any &&) noexcept = default;
		constexpr any &operator=(any &&other) noexcept
		{
			std::swap(m_type, other.m_type);
			std::swap(m_storage, other.m_storage);
			return *this;
		}

		/** Copies value of the managed object of \a other.
		 * @throw bad_argument_list If the underlying type of \a other is not copy-constructible. */
		any(const any &other) : any(other.type(), std::in_place, other.cdata()) {}
		/** If types of `this` and \a other are the same and `this` is not a reference, copy-assigns the managed object.
		 * Otherwise, destroys `this` and copy-constructs the managed object from \a other.
		 * @throw bad_argument_list If the managed object cannot be copy-assigned and the underlying type of \a other is not copy-constructible. */
		any &operator=(const any &other) { return assign(other.type(), std::in_place, other.cdata()); }

		~any() { destroy(); }

		/** Initializes `any` to manage a reference to \a ref. */
		template<typename T>
		any(T &ref) requires (!std::same_as<std::decay_t<T>, any>) : any(type_of(ref), ref) {}
		/** Initializes `any` to manage a reference to \a ref using type info \a type. */
		template<typename T>
		any(type_info type, T &ref) requires (!std::same_as<std::decay_t<T>, any>) : m_type(type)
		{
			if constexpr (!std::is_const_v<T>)
				external().data = std::addressof(ref);
			else
			{
				external().data = const_cast<std::remove_const_t<T> *>(std::addressof(ref));
				flags() = detail::IS_CONST;
			}
		}

		/** Initializes `any` to manage an instance of \a T move-constructed from \a value. */
		template<typename T>
		any(T &&value) requires (!std::same_as<std::decay_t<T>, any>) : any(std::in_place_type<std::decay_t<T>>, std::forward<T>(value)) {}
		/** Initializes `any` to manage an instance of \a T move-constructed from \a value using type info \a type. */
		template<typename T>
		any(type_info type, T &&value) requires (!std::same_as<std::decay_t<T>, any>) : any(type, std::in_place_type<std::decay_t<T>>, std::forward<T>(value)) {}

		/** Initializes `any` to manage in-place constructed instance of \a T with arguments \a args. */
		template<typename T, typename... Args>
		any(std::in_place_type_t<T>, Args &&...args) requires std::constructible_from<T, Args...> : any(type_info::get<T>(), std::in_place_type<T>, std::forward<Args>(args)...) {}
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
		any(type_info type, std::in_place_t, T *ptr, Del &&del = Del{}) requires(valid_deleter<Del>) : m_type(type)
		{
			flags() = detail::IS_OWNED;

			if constexpr (!(std::is_function_v<Del> && std::is_invocable_v<Del, void *>))
				external().deleter = +[](void *ptr) { Del{}(detail::void_cast<T>(ptr)); };
			else
				external().deleter = del;

			if constexpr (!std::is_const_v<T>)
				external().data = static_cast<void *>(ptr);
			else
			{
				external().data = const_cast<void *>(static_cast<const void *>(ptr));
				flags() |= detail::IS_CONST;
			}
		}

		/** Initializes `any` to manage a reference to object of type \a type located at \a ptr. */
		any(type_info type, void *ptr) noexcept : m_type(type)
		{
			external().data = ptr;
		}
		/** @copydoc any */
		any(type_info type, const void *ptr) noexcept : m_type(type)
		{
			flags() = detail::IS_CONST;
			external().data = const_cast<void *>(ptr);
		}

		/** Initializes `any` to manage an object of type \a type copy-constructed from value at \a ptr. */
		any(type_info type, std::in_place_t, void *ptr) { copy_init(type, ptr); }
		/** @copydoc any */
		any(type_info type, std::in_place_t, const void *ptr) { copy_init(type, ptr); }

		/** Initializes `any` to take ownership of \a ptr with deleter \a del using type info \a type. */
		any(type_info type, std::in_place_t, void *ptr, void (*del)(void *)) noexcept : m_type(type)
		{
			flags() = detail::IS_OWNED;
			external().deleter = del;
			external().data = ptr;
		}
		/** @copydoc any */
		any(type_info type, std::in_place_t, const void *ptr, void (*del)(const void *)) noexcept : m_type(type)
		{
			flags() = detail::IS_OWNED | detail::IS_CONST;
			external().deleter = reinterpret_cast<void (*)(void *)>(del);
			external().data = const_cast<void *>(ptr);
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
			m_type = type;
			if constexpr (!std::is_const_v<T>)
				external().data = std::addressof(ref);
			else
			{
				external().data = const_cast<std::remove_const_t<T> *>(std::addressof(ref));
				flags() = detail::IS_CONST;
			}
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
		[[nodiscard]] constexpr type_info type() const noexcept { return {m_type}; }

		/** Checks if the `any` has a managed object (either value or reference). */
		[[nodiscard]] constexpr bool empty() const noexcept { return !m_type.valid(); }
		/** Checks if the managed object is const-qualified. */
		[[nodiscard]] bool is_const() const noexcept { return flags() & detail::IS_CONST; }
		/** Checks if the `any` contains a reference to an external object. */
		[[nodiscard]] bool is_ref() const noexcept { return !(flags() & detail::IS_OWNED); }

		/** Returns a void pointer to the managed object.
		 * @note If the managed object is const-qualified, returns `nullptr`. */
		[[nodiscard]] void *data() noexcept
		{
			if (flags() & detail::IS_CONST) [[unlikely]] return nullptr;
			return (flags() & detail::IS_VALUE) ? local() : external().data;
		}
		/** Returns a const void pointer to the managed object. */
		[[nodiscard]] const void *cdata() const noexcept
		{
			return (flags() & detail::IS_VALUE) ? local() : external().data;
		}
		/** @copydoc cdata */
		[[nodiscard]] const void *data() const noexcept { return cdata(); }

		/** Resets `any` to an empty state. */
		void reset()
		{
			destroy();
			m_type = {};
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
				[[unlikely]] throw bad_any_cast(m_type, type);
			else
				return result;
		}
		/** @copydoc cast */
		[[nodiscard]] any cast(type_info type) const
		{
			if (auto result = try_cast(type); result.empty())
				[[unlikely]] throw bad_any_cast(m_type, type);
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
		[[nodiscard]] REFLEX_PUBLIC_OR_INLINE any try_cast(type_info type);
		/** @copydoc try_cast */
		[[nodiscard]] REFLEX_PUBLIC_OR_INLINE any try_cast(type_info type) const;

		/** Returns pointer to the managed object or `nullptr` if the managed object is of a different type or const-ness than `T`. */
		template<typename T>
		[[nodiscard]] T *get() noexcept
		{
			if constexpr (std::is_const_v<T>)
				return std::as_const(*this).get<T>();
			else if (type().name() == type_name_v<std::decay_t<T>>)
				return static_cast<T *>(data());
			else
				return nullptr;
		}
		/** Returns const pointer to the managed object or `nullptr` if the managed object is of a different type. */
		template<typename T>
		[[nodiscard]] std::add_const_t<T> *get() const noexcept
		{
			if (type().name() == type_name_v<std::decay_t<T>>)
				return static_cast<std::add_const_t<T> *>(data());
			else
				return nullptr;
		}

		/** Returns pointer to the managed object, cast to `T *` or `nullptr` if the managed object is of a different const-ness or not representable with `T *`. */
		template<typename T>
		[[nodiscard]] T *as()
		{
			if constexpr (std::is_const_v<T>)
				return std::as_const(*this).as<T>();
			else if (type().name() != type_name_v<std::decay_t<T>>)
				return static_cast<T *>(base_cast(type_name_v<std::decay_t<T>>));
			else
				return static_cast<T *>(data());
		}
		/** Returns const pointer to the managed object, cast to `T *` or `nullptr` if the managed object is of a different const-ness or not representable with `T *`. */
		template<typename T>
		[[nodiscard]] std::add_const_t<T> *as() const
		{
			if (type().name() != type_name_v<std::decay_t<T>>)
				return static_cast<std::add_const_t<T> *>(base_cast(type_name_v<std::decay_t<T>>));
			else
				return static_cast<std::add_const_t<T> *>(data());
		}

		/** Returns facet \a F for the managed object. */
		template<typename F>
		[[nodiscard]] inline F facet();

		/** If the managed object of `this` is equal to the managed object of \a other, or if `this` and \a other are empty,
		 * returns `true`. Otherwise returns `false`. */
		[[nodiscard]] REFLEX_PUBLIC_OR_INLINE bool operator==(const any &other) const;
		/** If the managed object of `this` is not equal to the managed object of \a other, returns `true`. Otherwise returns `false`.  */
		[[nodiscard]] REFLEX_PUBLIC_OR_INLINE bool operator!=(const any &other) const;
		/** If the managed object of `this` is greater than or equal to the managed object of \a other, or if `this` is not empty while
		 * \a other is empty, or if both `this` and \a other are empty, returns `true`. Otherwise returns `false`.  */
		[[nodiscard]] REFLEX_PUBLIC_OR_INLINE bool operator>=(const any &other) const;
		/** If the managed object of `this` is less than or equal to the managed object of \a other, or if \a other is not empty while
		 * `this` is empty, or if both `this` and \a other are empty, returns `true`. Otherwise returns `false`.  */
		[[nodiscard]] REFLEX_PUBLIC_OR_INLINE bool operator<=(const any &other) const;
		/** If the managed object of `this` is greater than the managed object of \a other, or if `this` is not empty while
		 * \a other is empty, returns `true`. Otherwise returns `false`.  */
		[[nodiscard]] REFLEX_PUBLIC_OR_INLINE bool operator>(const any &other) const;
		/** If the managed object of `this` is less than the managed object of \a other, or if \a other is not empty while
		 * `this` is empty, returns `true`. Otherwise returns `false`.  */
		[[nodiscard]] REFLEX_PUBLIC_OR_INLINE bool operator<(const any &other) const;

	private:
		[[nodiscard]] void *local() noexcept { return m_storage.bytes; }
		[[nodiscard]] const void *local() const noexcept { return m_storage.bytes; }
		[[nodiscard]] ptr_storage &external() noexcept { return *m_storage.get<ptr_storage, 0>(); }
		[[nodiscard]] const ptr_storage &external() const noexcept { return *m_storage.get<ptr_storage, 0>(); }

		[[nodiscard]] detail::type_flags &flags() noexcept
		{
			constexpr auto off = -static_cast<std::ptrdiff_t>(sizeof(detail::type_flags));
			return *m_storage.get<detail::type_flags, off>();
		}
		[[nodiscard]] const detail::type_flags &flags() const noexcept
		{
			constexpr auto off = -static_cast<std::ptrdiff_t>(sizeof(detail::type_flags));
			return *m_storage.get<detail::type_flags, off>();
		}

		template<typename T, typename... Args>
		void init_owned(type_info type, Args &&...args)
		{
			m_type = type;
			flags() = detail::IS_OWNED | (std::is_const_v<T> ? detail::IS_CONST : detail::type_flags{});
			if constexpr (!is_by_value<T>)
			{
				external().data = static_cast<void *>(new std::remove_cv_t<T>(std::forward<Args>(args)...));
				external().deleter = +[](void *ptr) { delete static_cast<std::remove_cv_t<T> *>(ptr); };
			}
			else
			{
				std::construct_at(std::launder(static_cast<T *>(local())), std::forward<Args>(args)...);
				flags() |= detail::IS_VALUE;
			}
		}
		template<typename T>
		inline void copy_init(type_info type, T *ptr);
		template<typename T>
		inline void copy_assign(type_info type, T *ptr);
		REFLEX_PUBLIC_OR_INLINE void destroy();

		template<typename T, typename U>
		void impl_copy_from(type_info type, U *data)
		{
			using other_t = take_const_t<T, U>;
			if constexpr (std::is_constructible_v<T, other_t &>)
				init_owned<T>(type, *static_cast<other_t *>(data));
			else
				throw detail::make_ctor_error<T, other_t &>();
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
			if (auto *tgt = get<T>(); is_ref() || tgt == nullptr)
				copy_from<T>(type, cdata, data);
			else if (cdata != nullptr)
				*tgt = *static_cast<std::add_const_t<T> *>(cdata);
			else
				*tgt = *detail::void_cast<T>(data);
		}

		[[nodiscard]] REFLEX_PUBLIC_OR_INLINE void *base_cast(std::string_view) const;
		[[nodiscard]] REFLEX_PUBLIC_OR_INLINE any value_conv(std::string_view) const;

		type_info m_type = {};
		storage_t m_storage;
	};

	namespace detail
	{
		[[nodiscard]] inline static bad_argument_list make_ctor_error(type_info type, std::span<any> args)
		{
			std::string msg;
			msg.append("Type `");
			msg.append(type.name());
			msg.append("` is not constructible from arguments {");

			/* Decorate args. */
			for (std::size_t i = 0; i < args.size();)
			{
				const auto &arg = args[i];
				if (arg.is_const())
					msg.append("const ");
				msg.append(arg.type().name());
				if (arg.is_ref())
					msg.append(" &");
				if (++i != args.size())
					msg.append(1, ',');
			}

			msg.append(1, '}');
			return bad_argument_list{msg};
		}
		template<typename T, typename... Ts>
		[[nodiscard]] inline static bad_argument_list make_ctor_error()
		{
			constexpr auto name_cs = []<typename U>(std::in_place_type_t<U>)
			{
				constexpr auto name = type_name_v<U>;
				return const_string<name.size()>{name};
			};

			constexpr auto msg = basic_const_string{"Type `"} + name_cs(std::in_place_type<T>) +
			                     basic_const_string{"` is not constructible from arguments {"} +
			                     (name_cs(std::in_place_type<Ts>) + ... + basic_const_string{"}"});
			return bad_argument_list{msg.data()};
		}
	}

	/** Returns the type info of the object managed by \a value. Equivalent to `value.type()`. */
	template<typename T>
	[[nodiscard]] inline type_info type_of(T &&value) requires std::same_as<std::decay_t<T>, any> { return value.type(); }

	/** Returns an `any` referencing the object managed by \a other. */
	template<typename T>
	[[nodiscard]] inline any forward_any(T &other) requires(std::same_as<std::decay_t<T>, any>) { return other.ref(); }
	/** Returns an `any` referencing the object at \a instance. */
	template<typename T>
	[[nodiscard]] inline any forward_any(T &instance) requires (!std::same_as<std::decay_t<T>, any>) { return any{std::forward<std::decay_t<T>>(instance)}; }

	/** Returns an `any` containing a move-constructed instance of \a value. */
	template<typename T>
	[[nodiscard]] inline any forward_any(T &&value) { return any{std::forward<std::decay_t<T>>(value)}; }
	/** Forwards \a value. */
	template<typename T>
	[[nodiscard]] inline any &&forward_any(any &&value) { return std::forward<any>(value); }

	/** Returns an `any` owning an instance of \a T constructed with arguments \a args. */
	template<typename T, typename... Args>
	[[nodiscard]] inline any make_any(Args &&...args) { return any{std::in_place_type<T>, std::forward<Args>(args)...}; }

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

	template<std::size_t N>
	bool type_info::construct_at(void *ptr, std::span<any, N> args) const { return construct_at(ptr, std::span<any>{args}); }
	template<typename... Args>
	bool type_info::construct_at(void *ptr, Args &&... args) const
	{
		if constexpr (sizeof...(Args) == 0)
			return construct_at(ptr, std::span<any>{});
		else
		{
			auto args_array = std::array{forward_any(args)...};
			return construct_at(ptr, std::span<any>{args_array});
		}
	}
}