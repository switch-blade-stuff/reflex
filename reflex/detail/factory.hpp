/*
 * Created by switchblade on 2023-03-26.
 */

#pragma once

#include "type_data.hpp"

namespace reflex
{
	/** Type factory used to initialize type info. */
	template<typename T>
	class type_factory
	{
		friend class type_info;
		template<typename>
		friend detail::type_data *detail::make_type_data(detail::database_impl &);

		type_factory(detail::type_handle handle, detail::database_impl &db) : m_data(handle(db)), m_db(&db) {}
		constexpr type_factory(detail::type_data *data, detail::database_impl *db) noexcept : m_data(data), m_db(db) {}

	public:
		type_factory() = delete;

		constexpr type_factory(const type_factory &) noexcept = default;
		constexpr type_factory &operator=(const type_factory &) noexcept = default;
		constexpr type_factory(type_factory &&) noexcept = default;
		constexpr type_factory &operator=(type_factory &&) noexcept = default;

		/** Returns the underlying type initialized by this type factory. */
		[[nodiscard]] constexpr type_info type() const noexcept { return {m_data, m_db}; }

		/** Adds enumeration constant named \a name constructed from arguments \a args to the underlying type info. */
		template<typename... Args>
		type_factory &enumerate(std::string_view name, Args &&...args) requires std::constructible_from<T, Args...>
		{
			m_data->conv_list.emplace_or_replace(name, type(), std::in_place_type<T>, std::forward<Args>(args)...);
			return *this;
		}

		/** Adds base type \a U to the list of bases of the underlying type info. */
		template<typename U>
		type_factory &base() requires std::derived_from<T, U>
		{
			m_data->base_list.emplace_or_replace(type_name_v<U>, detail::make_type_base<T, U>());
			return *this;
		}

		/** Makes the underlying type info convertible to \a U using conversion functor \a conv. */
		template<typename U, typename F>
		type_factory &conv(F &&conv) requires (std::invocable<F, const T &> && std::constructible_from<U, std::invoke_result_t<F, const T &>>)
		{
			m_data->conv_list.emplace_or_replace(type_name_v<U>, detail::make_type_conv<T, U>(std::forward<F>(conv)));
			return *this;
		}
		/** Makes the underlying type info convertible to \a U using `static_cast<U>(value)`. */
		template<typename U>
		type_factory &conv() requires (std::convertible_to<T, U> && std::same_as<std::decay_t<U>, U>)
		{
			m_data->conv_list.emplace_or_replace(type_name_v<U>, detail::make_type_conv<T, U>());
			return *this;
		}

		/** Makes the underlying type info constructible via constructor `T(Args...)`. */
		template<typename... Args>
		type_factory &ctor() requires std::constructible_from<T, Args...>
		{
			add_ctor<Args...>();
			return *this;
		}
		/** Makes the underlying type info constructible via factory function \a ctor_func.
		 * \a ctor_func must be invocable with arguments \a Args and return an instance of `any`. */
		template<typename... Args, typename F>
		type_factory &ctor(F &&ctor_func) requires std::is_invocable_r_v<any, F, Args...>
		{
			add_ctor<Args...>(std::forward<F>(ctor_func));
			return *this;
		}

		/** Adds a member object pointer \a M as a property with name \a name for the underlying type info.  */
		template<auto M>
		type_factory &prop(std::string_view name) requires (std::is_member_object_pointer_v<decltype(M)> && requires(T *p) { (p->*M); })
		{
			m_data->prop_list.emplace_or_replace(name, detail::make_type_prop<T, M>());
			return *this;
		}
		/** Adds a property defined by getter \a get_func and setter \a set_func with name \a name for the underlying type info.
		 * \a get_func must be invocable with a (possibly const-qualified) pointer to `T` and must return a non-void value, and
		 * is used to get value of the property. \a set_func must be invocable with a mutable pointer to `T` and an instance
		 * of `any`, and is used to set value of the property. */
		template<typename Fg, typename Fs>
		type_factory &prop(std::string_view name, Fg &&get_func, Fs &&set_func) requires (requires(Fg &&g, Fs &&s, T *p, any v) { forward_any(g(p)); s(p, std::move(v)); })
		{
			m_data->prop_list.emplace_or_replace(name, detail::make_type_prop<T>(std::forward<Fg>(get_func), std::forward<Fs>(set_func)));
			return *this;
		}

		/** Implements facet \a F for the underlying type info.
		 * `impl_facet<F, T>` must be well-defined and have static member constant `value` of type `F::vtable_type`. */
		template<typename F>
		inline type_factory &facet();
		/** Implements facet \a F for the underlying type info using facet vtable \a vtab.
		 * @param vtab Reference to facet vtable for facet \a F. */
		template<typename F>
		inline type_factory &facet(const typename F::vtable_type &vtab);

		/** Implements all facets in facet group \a G for the underlying type info.
		 * `impl_facet<F, T>` must be well-defined and have static member constant `value` of type `F::vtable_type`
		 * for every facet type `F` in facet group \a G. */
		template<template_instance<facet_group> G>
		inline type_factory &facet();
		/** Implements all facets in facet group \a G for the underlying type info using group vtable \a vtab.
		 * @param vtab Tuple of vtables of the individual facet types in facet group \a G. */
		template<template_instance<facet_group> G>
		inline type_factory &facet(const typename G::vtable_type &vtab);

	private:
		template<typename... Ts, typename... Args>
		void add_ctor(Args &&...args)
		{
			constexpr auto args_data = detail::arg_list<Ts...>::value;
			if (auto existing = m_data->find_ctor(args_data); existing == nullptr)
				m_data->ctor_list.emplace_back(detail::make_type_ctor<T, Ts...>(std::forward<Args>(args)...));
			else
				*existing = detail::make_type_ctor<T, Ts...>(std::forward<Args>(args)...);
		}

		template<typename... Vt>
		inline void add_facet(const std::tuple<const Vt *...> &vtab);

		detail::type_data *m_data;
		detail::database_impl *m_db;
	};

	/** @brief User customization object used to hook into type info initialization.
	 *
	 * Specializations of `type_init` must provide a member function `void operator(type_factory<T>) const`.
	 * This function is invoked at type info initialization time, and is used to provide the user with
	 * ability to customize the default type info object. */
	template<typename T>
	struct type_init;

#ifndef REFLEX_NO_ARITHMETIC_CONV
	/** Specialization of `type_init` for arithmetic types. */
	template<typename T> requires std::is_arithmetic_v<T>
	struct type_init<T>
	{
	private:
		template<typename U>
		static void make_convertible(type_factory<T> factory)
		{
			if constexpr (!std::same_as<T, U>)
			{
				if constexpr (std::constructible_from<T, U>)
					factory.template ctor<U>([](auto &&value) { return make_any<T>(static_cast<T>(value)); });
				if constexpr (std::convertible_to<T, U>)
					factory.template conv<U>();
			}
		}
		template<typename... Ts>
		static void make_convertible(type_pack_t<Ts...>, type_factory<T> factory)
		{
			(make_convertible<Ts>(factory), ...);
		}

	public:
		void operator()(type_factory<T> factory) const
		{
			make_convertible<bool>(factory);
			make_convertible(unique_type_pack<type_pack_t<
					char, wchar_t, char8_t, char16_t, char32_t,
					std::int8_t, std::int16_t, std::int32_t, std::int64_t,
					std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
					std::intmax_t, std::uintmax_t, std::intptr_t, std::uintptr_t,
					std::ptrdiff_t, std::size_t>>, factory);
			make_convertible(type_pack<float, double, long double>, factory);
		}
	};
#endif
}