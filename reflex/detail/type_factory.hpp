/*
 * Created by switchblade on 2023-03-26.
 */

#pragma once

#include "type_data.hpp"
#include "facet.hpp"

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
			m_data->conv_list.try_emplace(name, type(), std::in_place_type<T>, std::forward<Args>(args)...);
			return *this;
		}

		/** Adds base type \a U to the list of bases of the underlying type info. */
		template<typename U>
		type_factory &base() requires std::derived_from<T, U>
		{
			if (const auto name = type_name_v<U>; !m_data->base_list.contains(name))
				m_data->base_list.emplace(name, detail::make_type_base<T, U>());
			return *this;
		}

		/** Makes the underlying type info convertible to \a U using conversion functor \a conv. */
		template<typename U, typename F>
		type_factory &conv(F &&conv) requires (std::invocable<F, const T &> && std::constructible_from<U, std::invoke_result_t<F, const T &>>)
		{
			if (const auto name = type_name_v<U>; !m_data->conv_list.contains(name))
				m_data->conv_list.emplace(name, detail::make_type_conv<T, U>(std::forward<F>(conv)));
			return *this;
		}
		/** Makes the underlying type info convertible to \a U using `static_cast<U>(value)`. */
		template<typename U>
		type_factory &conv() requires (std::convertible_to<T, U> && std::same_as<std::decay_t<U>, U>)
		{
			if (const auto name = type_name_v<U>; !m_data->conv_list.contains(name))
				m_data->conv_list.emplace(name, detail::make_type_conv<T, U>());
			return *this;
		}

		/** Makes the underlying type info constructible via constructor `T(Args...)`. */
		template<typename... Args>
		type_factory &ctor() requires std::constructible_from<T, Args...>
		{
			add_ctor<Args...>();
			return *this;
		}
		/** Makes the underlying type info constructible via allocating & placement factory functions \a alloc_func and \a place_func.
		 * \a alloc_func must be invocable with arguments \a Args and return an instance of `any`, and
		 * \a place_func must be invocable with pointer to \a T, and arguments \a Args. */
		template<typename... Args, typename Fa, typename Fp>
		type_factory &ctor(Fa &&alloc_func, Fp &&place_func) requires (std::is_invocable_r_v<any, Fa, Args...> && std::is_invocable_v<Fp, T *, Args...>)
		{
			add_ctor<Args...>(std::forward<Fa>(alloc_func), std::forward<Fp>(place_func));
			return *this;
		}

	private:
		template<typename... Ts, typename... Args>
		void add_ctor(Args &&...args)
		{
			constexpr auto args_data = detail::arg_list<Ts...>::value;
			if (m_data->find_ctor(args_data) != nullptr) return;

			m_data->ctor_list.emplace_back(detail::make_type_ctor<T, Ts...>(std::forward<Args>(args)...));
		}

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

	/** Specialization of `type_init` for arithmetic types. */
	template<typename T> requires std::is_arithmetic_v<T>
	struct type_init<T>
	{
	private:
		template<typename U>
		static void make_convertible(type_factory<T> factory)
		{
			if constexpr (!std::same_as<T, U> && std::convertible_to<T, U>)
				factory.template conv<U>();
		}

	public:
		void operator()(type_factory<T> factory) const
		{
			/* Conversion to bool. */
			make_convertible<bool>(factory);

			/* Conversions to characters. */
			make_convertible<char>(factory);
			make_convertible<wchar_t>(factory);
			make_convertible<char8_t>(factory);
			make_convertible<char16_t>(factory);
			make_convertible<char32_t>(factory);

			/* Conversions to integers. */
			make_convertible<std::int8_t>(factory);
			make_convertible<std::int16_t>(factory);
			make_convertible<std::int32_t>(factory);
			make_convertible<std::int64_t>(factory);
			make_convertible<std::uint8_t>(factory);
			make_convertible<std::uint16_t>(factory);
			make_convertible<std::uint32_t>(factory);
			make_convertible<std::uint64_t>(factory);

			/* Conversions to floats. */
			make_convertible<float>(factory);
			make_convertible<double>(factory);
			make_convertible<long double>(factory);
		}
	};
}