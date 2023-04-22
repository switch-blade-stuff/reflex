/*
 * Created by switchblade on 2023-03-26.
 */

#pragma once

#include "data.hpp"

namespace reflex
{
	/** Type factory implementation for synthetic types. */
	template<>
	class type_factory<>
	{
		friend struct detail::type_data;
		template<typename>
		friend class type_factory;
		friend class type_info;

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

		/** Adds an attribute initialized from \a value to the underlying type info. */
		type_factory &attribute(const any &value)
		{
			m_data->attrs.emplace_or_replace(value.type().name(), value);
			return *this;
		}
		/** @copydoc attribute. */
		type_factory &attribute(any &&value)
		{
			m_data->attrs.emplace_or_replace(value.type().name(), std::forward<any>(value));
			return *this;
		}

		/** Adds an enumeration constant named \a name initialized from \a value to the underlying type info. */
		type_factory &enumerate(std::string_view name, const any &value)
		{
			m_data->enums.emplace_or_replace(name, value);
			return *this;
		}
		/** @copydoc enumerate. */
		type_factory &enumerate(std::string_view name, any &&value)
		{
			m_data->enums.emplace_or_replace(name, std::forward<any>(value));
			return *this;
		}

		/** Adds a parent of type \a type to the list of bases of the underlying type info.
		 * Dynamic casts from child type to parent type will be preformed using \a cast.  */
		template<typename F>
		type_factory &add_parent(type_info type, F &&cast) requires std::is_invocable_r_v<const void *, F, const void *>
		{
			m_data->bases.emplace_or_replace(type.name(), [n = type.name()](auto &db) { return detail::data_factory(n, db); }, std::forward<F>(cast));
			return *this;
		}

		/** Implements facet \a F for the underlying type info using facet vtable \a vtab.
		 * @param vtab Reference to facet vtable for facet \a F. */
		template<typename F>
		inline type_factory &implement_facet(const typename F::vtable_type &vtab);
		/** Implements all facets in facet group \a G for the underlying type info using group vtable \a vtab.
		 * @param vtab Tuple of vtables of the individual facet types in facet group \a G. */
		template<instance_of<facets::facet_group> G>
		inline type_factory &implement_facet(const typename G::vtable_type &vtab);

		/** Makes the underlying type info constructible via factory function \a ctor_func.
		 * \a ctor_func must be invocable with \a Args. */
		template<typename... Args, typename F>
		type_factory &make_constructible(F &&ctor_func) requires std::is_invocable_v<F, Args...>
		{
			add_ctor<Args...>(std::forward<F>(ctor_func));
			return *this;
		}
		/** Makes the underlying type info constructible via factory function \a ctor_func.
		 * \a ctor_func must be invocable with a span of `any` matching arguments \a args and return an instance of `any`. */
		template<typename F>
		type_factory &make_constructible(const argument_list &args, F &&ctor_func) requires std::is_invocable_r_v<any, F, std::span<any>>
		{
			add_ctor(args.m_data, std::forward<F>(ctor_func));
			return *this;
		}

		/** Makes the underlying type info convertible to type \a type using conversion functor \a conv.
		 * @note Conversion to the underlying type of enums is added by default when available. */
		template<typename F>
		type_factory &make_convertible(type_info type, F &&conv) requires std::is_invocable_r_v<any, F, const void *>
		{
			m_data->convs.emplace_or_replace(type.name(), std::forward<F>(conv));
			return *this;
		}
		
	private:
		template<typename... Vs>
		inline void add_facet(const std::tuple<const Vs *...> &vt);

		template<typename... Ts, typename F>
		void add_ctor(F &&func)
		{
			const auto args = std::array{detail::make_arg_data<Ts>()...};
			if (auto existing = m_data->find_exact_ctor(args); existing == nullptr)
				m_data->ctors.emplace_back(type_pack<Ts...>, std::forward<F>(func));
			else
				*existing = detail::type_ctor{type_pack<Ts...>, std::forward<F>(func)};
		}
		template<typename F>
		void add_ctor(std::span<const detail::arg_data> args, F &&func)
		{
			if (auto existing = m_data->find_exact_ctor(args); existing == nullptr)
				m_data->ctors.emplace_back(args, std::forward<F>(func));
			else
				*existing = detail::type_ctor{args, std::forward<F>(func)};
		}

		detail::type_data *m_data;
		detail::database_impl *m_db;
	};

	/** Type factory used to initialize type info for type \a T. */
	template<typename T>
	class type_factory : public type_factory<>
	{
	public:
		using type_factory<>::type_factory;
		using type_factory<>::operator=;

		using type_factory<>::make_constructible;
		using type_factory<>::make_convertible;
		using type_factory<>::implement_facet;
		using type_factory<>::add_parent;
		using type_factory<>::enumerate;
		using type_factory<>::attribute;

		/** Adds an attribute of type \a A constructed from arguments \a args to the underlying type info. If attribute
		 * type \a A is constructible from `type_factory, Args...`, injects the type factory instance as the first argument.
		 * This can be used to inject dynamic type metadata on a per-attribute, rather than per-type basis. */
		template<typename A, typename... Args>
		type_factory &attribute(Args &&...args) requires (std::same_as<std::decay_t<A>, A> && (std::constructible_from<A, Args...> || std::constructible_from<A, type_factory &, Args...>))
		{
			if constexpr (std::constructible_from<A, type_factory &, Args...>)
				m_data->attrs.emplace_or_replace(type_name_v<A>, type(), std::in_place_type<T>, *this, std::forward<Args>(args)...);
			else
				m_data->attrs.emplace_or_replace(type_name_v<A>, type(), std::in_place_type<T>, std::forward<Args>(args)...);
			return *this;
		}
		/** Adds enumeration constant named \a name constructed from arguments \a args to the underlying type info. */
		template<typename... Args>
		type_factory &enumerate(std::string_view name, Args &&...args) requires std::constructible_from<T, Args...>
		{
			m_data->enums.emplace_or_replace(name, type(), std::in_place_type<T>, std::forward<Args>(args)...);
			return *this;
		}
		/** Adds enumeration constant named \a name initialized from \a Value to the underlying type info. */
		template<auto Value>
		type_factory &enumerate(std::string_view name) { return enumerate(name, Value); }

		/** Adds base type \a U to the list of bases of the underlying type info. */
		template<typename U>
		type_factory &add_parent() requires std::derived_from<T, U>
		{
			m_data->bases.emplace_or_replace(type_name_v<U>, detail::make_type_base<T, U>());
			return *this;
		}

		/** Implements facet \a F for the underlying type info.
		 * `impl_facet<F, T>` must be well-defined and have static member constant `value` of type `F::vtable_type`. */
		template<typename F>
		inline type_factory &implement_facet();
		/** Implements all facets in facet group \a G for the underlying type info.
		 * `impl_facet<F, T>` must be well-defined and have static member constant `value` of type `F::vtable_type`
		 * for every facet type `F` in facet group \a G. */
		template<instance_of<facets::facet_group> G>
		inline type_factory &implement_facet();

		/** Makes the underlying type info constructible via constructor `T(Args...)`.
		 * @note Construction from the underlying type of enums is added by default when available. */
		template<typename... Args>
		type_factory &make_constructible() requires std::constructible_from<T, Args...>
		{
			const auto args = std::array{detail::make_arg_data<Args>()...};
			if (auto existing = m_data->find_exact_ctor(args); existing == nullptr)
				m_data->ctors.emplace_back(detail::make_type_ctor<T, Args...>());
			else
				*existing = detail::make_type_ctor<T, Args...>();
			return *this;
		}

		/** Makes the underlying type info convertible to type \a type using conversion functor \a conv.
		 * @note Conversion to the underlying type of enums is added by default when available. */
		template<typename F>
		type_factory &make_convertible(type_info type, F &&conv) requires std::is_invocable_r_v<any, F, const T &>
		{
			m_data->convs.emplace_or_replace(type.name(), [f = std::forward<F>(conv)](const void *p) -> any
			{
				return forward_any(std::invoke(f, *static_cast<const T *>(p)));
			});
			return *this;
		}
		/** Makes the underlying type info convertible to type \a U using conversion functor \a conv.
		 * @note Conversion to the underlying type of enums is added by default when available. */
		template<typename U, typename F>
		type_factory &make_convertible(F &&conv) requires (std::same_as<std::decay_t<U>, U> && std::is_invocable_r_v<U, F, const T &>)
		{
			m_data->convs.emplace_or_replace(type_name_v<U>, detail::make_type_conv<T, U>(std::forward<F>(conv)));
			return *this;
		}
		/** Makes the underlying type info convertible to \a U using `static_cast<U>(value)`.
		 * @note Conversion to the underlying type of enums is added by default when available. */
		template<typename U>
		type_factory &make_convertible() requires (std::same_as<std::decay_t<U>, U> && std::convertible_to<T, U>)
		{
			m_data->convs.emplace_or_replace(type_name_v<U>, detail::make_type_conv<T, U>());
			return *this;
		}

	private:
	};

	/** @brief User customization object used to hook into type info initialization.
	 *
	 * Specializations of `type_init` must provide a member function `void operator(type_factory<T>) const`.
	 * This function is invoked at type info initialization time, and is used to customize the default type info object. */
	template<typename T>
	struct type_init;

#ifndef REFLEX_NO_ARITHMETIC
	namespace detail
	{
		template<typename T>
		REFLEX_PUBLIC void init_arithmetic(type_factory<T>);

		extern template void init_arithmetic<bool>(type_factory<bool>);

		extern template void init_arithmetic<char>(type_factory<char>);
		extern template void init_arithmetic<wchar_t>(type_factory<wchar_t>);
		extern template void init_arithmetic<char8_t>(type_factory<char8_t>);
		extern template void init_arithmetic<char16_t>(type_factory<char16_t>);
		extern template void init_arithmetic<char32_t>(type_factory<char32_t>);

		extern template void init_arithmetic<std::int8_t>(type_factory<std::int8_t>);
		extern template void init_arithmetic<std::int16_t>(type_factory<std::int16_t>);
		extern template void init_arithmetic<std::int32_t>(type_factory<std::int32_t>);
		extern template void init_arithmetic<std::int64_t>(type_factory<std::int64_t>);
		extern template void init_arithmetic<std::uint8_t>(type_factory<std::uint8_t>);
		extern template void init_arithmetic<std::uint16_t>(type_factory<std::uint16_t>);
		extern template void init_arithmetic<std::uint32_t>(type_factory<std::uint32_t>);
		extern template void init_arithmetic<std::uint64_t>(type_factory<std::uint64_t>);

		extern template void init_arithmetic<std::intptr_t>(type_factory<std::intptr_t>);
		extern template void init_arithmetic<std::uintptr_t>(type_factory<std::uintptr_t>);

		extern template void init_arithmetic<std::intmax_t>(type_factory<std::intmax_t>);
		extern template void init_arithmetic<std::uintmax_t>(type_factory<std::uintmax_t>);

		extern template void init_arithmetic<std::ptrdiff_t>(type_factory<std::ptrdiff_t>);
		extern template void init_arithmetic<std::size_t>(type_factory<std::size_t>);

		extern template void init_arithmetic<float>(type_factory<float>);
		extern template void init_arithmetic<double>(type_factory<double>);
		extern template void init_arithmetic<long double>(type_factory<long double>);
	}

	/** Specialization of `type_init` for arithmetic types. */
	template<typename T> requires std::is_arithmetic_v<T>
	struct type_init<T> { void operator()(type_factory<T> f) const { detail::init_arithmetic(f); } };
#endif
}