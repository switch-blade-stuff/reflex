/*
 * Created by switchblade on 2023-03-11.
 */

#pragma once

#include "type_domain.hpp"
#include "type_info.hpp"
#include "arg_info.hpp"

namespace reflex
{
	/** Type-erased generic object. */
	class any
	{
		friend struct detail::type_data;
		friend class type_info;

		using value_storage = std::uint8_t[sizeof(std::intptr_t) * 2];
		struct ptr_storage
		{
			union
			{
				void (*deleter)(void *) = nullptr;
				void (*cdeleter)(const void *);
			};
			union
			{
				void *data = nullptr;
				const void *cdata;
			};
		};

		template<typename T>
		static constexpr auto is_by_value = alignof(T) <= alignof(ptr_storage) && sizeof(T) <= sizeof(value_storage);

	public:
		/** Initializes an empty `any`. */
		constexpr any() noexcept : m_value{} {}
		constexpr ~any() { destroy(); }

		/** Initializes `any` to manage a reference to \a ref using the global domain. */
		template<typename T>
		any(T &ref) : any(type_info::get<T>(), ref) {}
		/** Initializes `any` to manage a reference to \a ref using type info \a type. */
		template<typename T>
		any(type_info type, T &ref) : m_type(type)
		{
			m_external.data = std::addressof(ref);
			init_flags<T &>();
		}
		/** Initializes `any` to manage a reference to \a ref using type domain \a domain. */
		template<typename T>
		any(type_domain &domain, T &ref) : any(type_info::get<T>(domain), ref) {}

		/** Initializes `any` to take ownership of \a ptr with deleter \a Del using the global domain.
		 * @note Deleter functor must be empty. */
		template<typename T, typename Del = std::default_delete<T>>
		any(std::in_place_t, T *ptr, Del &&del = Del{}) requires std::is_empty_v<Del> : any(std::in_place, type_info::get<T>(), ptr, std::forward<Del>(del)) {}
		/** Initializes `any` to take ownership of \a ptr with deleter \a Del using type info \a type.
		 * @note Deleter functor must be empty. */
		template<typename T, typename Del = std::default_delete<T>>
		any(type_info type, std::in_place_t, T *ptr, Del && = Del{}) requires std::is_empty_v<Del> : m_type(type), m_flags(detail::IS_OWNED)
		{
			if constexpr (!std::is_const_v<T>)
			{
				m_external.deleter = +[](void *ptr) { Del{}(static_cast<T *>(ptr)); };
				m_external.data = static_cast<void *>(ptr);
			}
			else
			{
				m_external.cdeleter = +[](const void *ptr) { Del{}(static_cast<T *>(ptr)); };
				m_external.cdata = static_cast<const void *>(ptr);
				m_flags |= detail::IS_CONST;
			}
		}
		/** Initializes `any` to take ownership of \a ptr with deleter \a Del using type domain \a domain.
		 * @note Deleter functor must be empty. */
		template<typename T, typename Del = std::default_delete<T>>
		any(type_domain &domain, std::in_place_t, T *ptr, Del &&del = Del{}) requires std::is_empty_v<Del> : any(std::in_place, type_info::get<T>(domain), ptr, std::forward<Del>(del)) {}

		/** Initializes `any` to manage in-place constructed instance of \a T with arguments \a args using the global domain. */
		template<typename T, typename... Args>
		any(std::in_place_type_t<T>, Args &&...args) requires std::constructible_from<T, Args...> : any(std::in_place_type<T>, type_info::get<T>(), std::forward<Args>(args)...) {}
		/** Initializes `any` to manage in-place constructed instance of \a T with arguments \a args using type info \a type. */
		template<typename T, typename... Args>
		any(type_info type, std::in_place_type_t<T>, Args &&...args) requires std::constructible_from<T, Args...> { init_owned<T>(type, std::forward<Args>(args)...); }
		/** Initializes `any` to manage in-place constructed instance of \a T with arguments \a args using type domain \a domain. */
		template<typename T, typename... Args>
		any(type_domain &domain, std::in_place_type_t<T>, Args &&...args) requires std::constructible_from<T, Args...> : any(std::in_place_type<T>, type_info::get<T>(domain), std::forward<Args>(args)...) {}

		/** Initializes `any` to manage a reference to object of type \a type located at \a ptr. */
		constexpr any(type_info type, void *ptr) noexcept : m_type(type) { m_external.data = ptr; }
		/** @copydoc any */
		constexpr any(type_info type, const void *ptr) noexcept : m_type(type), m_flags(detail::IS_CONST) { m_external.cdata = ptr; }

		/** Initializes `any` to manage an object of type \a type copy-constructed from value at \a ptr. */
		any(type_info type, std::in_place_t, void *ptr) { (this->*type.m_data->any_copy_init)(type, nullptr, ptr); }
		/** @copydoc any */
		any(type_info type, std::in_place_t, const void *ptr) { (this->*type.m_data->any_copy_init)(type, ptr, nullptr); }

		/** Initializes `any` to take ownership of \a ptr with deleter \a del using type \a type. */
		constexpr any(type_info type, std::in_place_t, void *ptr, void (*del)(void *)) noexcept : m_type(type), m_flags(detail::IS_OWNED)
		{
			m_external.deleter = del;
			m_external.data = ptr;
		}
		/** @copydoc any */
		constexpr any(type_info type, std::in_place_t, const void *ptr, void (*del)(const void *)) noexcept : m_type(type), m_flags(static_cast<detail::type_flags>(detail::IS_OWNED | detail::IS_CONST))
		{
			m_external.cdeleter = del;
			m_external.cdata = ptr;
		}

		/** Returns type of the managed object. */
		[[nodiscard]] constexpr type_info type() const noexcept { return m_type; }

		/** Checks if the `any` has a managed object (either value or reference). */
		[[nodiscard]] constexpr bool empty() const noexcept { return !m_type.valid(); }
		/** Checks if the managed object is const-qualified. */
		[[nodiscard]] constexpr bool is_const() const noexcept { return m_flags & detail::IS_CONST; }
		/** Checks if the `any` contains a reference to an external object. */
		[[nodiscard]] constexpr bool is_ref() const noexcept { return !(m_flags & detail::IS_OWNED); }

		/** Returns a void pointer to the managed object.
		 * @note If the managed object is const-qualified, returns `nullptr`. */
		[[nodiscard]] constexpr void *data() noexcept
		{
			if (m_flags & detail::IS_CONST) [[unlikely]] return nullptr;
			return (m_flags & detail::IS_VALUE) ? m_value : m_external.data;
		}
		/** Returns a const void pointer to the managed object. */
		[[nodiscard]] constexpr const void *cdata() const noexcept
		{
			return (m_flags & detail::IS_VALUE) ? m_value : m_external.cdata;
		}
		/** @copydoc cdata */
		[[nodiscard]] constexpr const void *data() const noexcept { return cdata(); }

		/** Resets `any` to an empty state. */
		void reset()
		{
			destroy();
			m_type = {};
		}

		/** Returns pointer to the managed object or `nullptr` if the managed object is of a different type or const-ness than `T`. */
		template<typename T>
		[[nodiscard]] T *get() noexcept
		{
			if constexpr (std::is_const_v<T>)
				return std::as_const(*this).get<T>();
			else if (!is_const() && type().name() == type_name_v<T>)
				return static_cast<T *>(data());
			else
				return nullptr;
		}
		/** Returns const pointer to the managed object or `nullptr` if the managed object is of a different type. */
		template<typename T>
		[[nodiscard]] std::add_const_t<T> *get() const noexcept
		{
			if (type().name() == type_name_v<T>)
				return static_cast<std::add_const_t<T> *>(data());
			else
				return nullptr;
		}

		/** Returns pointer to the managed object, casted to `T *` or `nullptr` if the managed object is of a different const-ness or not convertible to `T *`. */
		template<typename T>
		[[nodiscard]] inline T *as() noexcept;
		/** Returns const pointer to the managed object, casted to `T *` or `nullptr` if the managed object is of a different const-ness or not convertible to `T *`. */
		template<typename T>
		[[nodiscard]] inline std::add_const_t<T> *as() const noexcept;

	private:
		template<typename T>
		constexpr void init_flags() noexcept
		{
			m_flags = std::is_reference_v<T> ? detail::IS_OWNED : detail::type_flags{};
			if constexpr (is_by_value<std::remove_reference_t<T>>) m_flags |= detail::IS_VALUE;
			if constexpr (std::is_const_v<std::remove_reference_t<T>>) m_flags |= detail::IS_CONST;
		}
		template<typename T, typename... Args>
		void init_owned(type_info type, Args &&...args)
		{
			m_type = type;
			if constexpr (is_by_value<T>)
				std::construct_at(std::launder(reinterpret_cast<T *>(m_value)), std::forward<Args>(args)...);
			else
				m_external.data = static_cast<void *>(new std::remove_cv_t<T>(std::forward<Args>(args)...));
			init_flags<T>();
		}

		template<typename T>
		void copy_init_from(type_info type, const void *cdata, void *data)
		{
			if (cdata != nullptr)
			{
				if constexpr (!std::is_constructible_v<T, const T &>)
					throw make_constructor_error<T, const T &>();
				init_owned<T>(type, *static_cast<std::add_const_t<T> *>(cdata));
			}
			else
			{
				if constexpr (!std::is_constructible_v<T, T &>)
					throw make_constructor_error<T, T &>();
				init_owned<T>(type, *static_cast<T *>(data));
			}
		}
		template<typename T>
		void copy_assign_from(type_info, const void *, void *)
		{
			/* TODO: Implement. */
		}

		void destroy()
		{
			/* Bail if empty or non-owning. */
			if (empty() || is_ref()) return;

			if (m_flags & detail::IS_VALUE)
				m_type.m_data->dtor.destroy_in_place(m_value);
			else if (m_external.deleter == nullptr)
				m_type.m_data->dtor.destroy_deleting(m_external.data);
			else
			{
				m_type.m_data->dtor.destroy_in_place(m_external.data);
				m_external.deleter(m_external.data);
			}
		}

		type_info m_type = {};
		union
		{
			value_storage m_value = {};
			ptr_storage m_external;
		};
		detail::type_flags m_flags;
	};

	namespace detail
	{
		constexpr bool argument_data::verify(std::span<const argument_data *const> expected, std::span<any> actual) noexcept
		{
			if (expected.size() != actual.size())
				return false;

			for (std::size_t i = 0; i < expected.size(); ++i)
			{
				if (actual[i].is_const() > (expected[i]->flags & IS_CONST) ||
				    expected[i]->type_name != actual[i].type().name())
					return false;
			}
			return true;
		}
		constexpr bool argument_data::verify(std::span<const argument_data *const> expected, std::span<const argument_data *const> actual) noexcept
		{
			if (expected.size() != actual.size())
				return false;

			for (std::size_t i = 0; i < expected.size(); ++i)
			{
				if ((actual[i]->flags & IS_CONST) > (expected[i]->flags & IS_CONST) ||
				    expected[i]->type_name != actual[i]->type_name)
					return false;
			}
			return true;
		}

		template<typename T>
		type_data *type_data::bind(type_domain &domain) noexcept
		{
			return domain.try_reflect(type_name<T>::value, +[]() noexcept -> type_data
			{
				type_data result;
				result.name = type_name_v<T>;

				if constexpr (std::same_as<T, void>)
					result.flags |= type_flags::IS_VOID;
				else
				{
					if constexpr (!std::is_empty_v<T>) result.size = sizeof(T);

					if constexpr (std::is_null_pointer_v<T>) result.flags |= type_flags::IS_NULL;
					if constexpr (std::is_enum_v<T>) result.flags |= type_flags::IS_ENUM;
					if constexpr (std::is_class_v<T>) result.flags |= type_flags::IS_CLASS;
					if constexpr (std::is_pointer_v<T>) result.flags |= type_flags::IS_POINTER;

					if constexpr (std::signed_integral<T> || std::convertible_to<T, std::intmax_t>)
					{
						result.flags |= type_flags::IS_SIGNED_INT;
						/* TODO: Add std::intmax_t conversion. */
					}
					if constexpr (std::unsigned_integral<T> || std::convertible_to<T, std::uintmax_t>)
					{
						result.flags |= type_flags::IS_UNSIGNED_INT;
						/* TODO: Add std::uintmax_t conversion. */
					}
					if constexpr (std::is_arithmetic_v<T> || std::convertible_to<T, double>)
					{
						result.flags |= type_flags::IS_ARITHMETIC;
						/* TODO: Add double conversion. */
					}

					result.remove_pointer = bind<std::remove_pointer_t<T>>;
					result.remove_extent = bind<std::remove_extent_t<T>>;
					result.extent = std::extent_v<T>;

					/* Constructors & destructors are only created for object types. */
					if constexpr (std::is_object_v<T>)
					{
						result.any_copy_init = &any::copy_init_from<T>;
						result.any_copy_assign = &any::copy_assign_from<T>;

						result.dtor = type_dtor::bind<T>();

						result.copy_ctor = type_ctor<>::bind<T>();
						result.default_ctor = type_ctor<>::bind<T>();
						result.default_ctor->next = result.copy_ctor;
						result.ctor_list = result.default_ctor;
					}
				}
				return result;
			});
		}
	}

	/** Returns an `any` referencing the object at \a instance using the global domain. */
	template<typename T>
	[[nodiscard]] inline any forward_any(T &instance) { return any{instance}; }
	/** Returns an `any` referencing the object at \a instance using type domain \a domain. */
	template<typename T>
	[[nodiscard]] inline any forward_any(const type_domain &domain, T &instance) { return any{domain, instance}; }

	/** Returns an `any` owning an instance of \a T constructed with arguments \a args using the global domain. */
	template<typename T, typename... Args>
	[[nodiscard]] inline any make_any(Args &&...args) { return any{std::in_place_type<T>, std::forward<Args>(args)...}; }
	/** Returns an `any` owning an instance of \a T constructed with arguments \a args using type domain \a domain. */
	template<typename T, typename... Args>
	[[nodiscard]] inline any make_any(const type_domain &domain, Args &&...args) { return any{std::in_place_type<T>, domain, std::forward<Args>(args)...}; }
}