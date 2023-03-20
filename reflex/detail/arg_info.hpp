/*
 * Created by switchblade on 2023-03-15.
 */

#pragma once

#include <ranges>

#include "any.hpp"

namespace reflex
{
	namespace detail
	{
		class argument_info_iterator
		{
			friend class reflex::argument_info;

			using iter_t = std::ranges::iterator_t<std::span<const detail::argument_data *const>>;

		public:
			class pointer;
			using value_type = argument_info;
			using reference = const value_type &;
			using difference_type = std::iter_difference_t<iter_t>;

		private:
			constexpr argument_info_iterator(iter_t &&iter, type_domain *domain = nullptr) noexcept : m_iter(std::forward<iter_t>(iter)), m_domain(domain) {}

		public:
			constexpr argument_info_iterator() noexcept = default;
			constexpr argument_info_iterator(const argument_info_iterator &) noexcept = default;
			constexpr argument_info_iterator(argument_info_iterator &&) noexcept = default;
			constexpr argument_info_iterator &operator=(const argument_info_iterator &) noexcept = default;
			constexpr argument_info_iterator &operator=(argument_info_iterator &&) noexcept = default;

			constexpr argument_info_iterator &operator++() noexcept
			{
				++m_iter;
				return *this;
			}
			constexpr argument_info_iterator &operator--() noexcept
			{
				--m_iter;
				return *this;
			}
			constexpr argument_info_iterator &operator+=(difference_type n) noexcept
			{
				m_iter += n;
				return *this;
			}
			constexpr argument_info_iterator &operator-=(difference_type n) noexcept
			{
				m_iter -= n;
				return *this;
			}

			[[nodiscard]] constexpr argument_info_iterator operator++(int) noexcept { return {m_iter++, m_domain}; }
			[[nodiscard]] constexpr argument_info_iterator operator--(int) noexcept { return {m_iter--, m_domain}; }

			[[nodiscard]] constexpr argument_info_iterator operator+(difference_type n) const noexcept { return {m_iter + n, m_domain}; }
			[[nodiscard]] constexpr argument_info_iterator operator-(difference_type n) const noexcept { return {m_iter - n, m_domain}; }
			[[nodiscard]] constexpr difference_type operator-(const argument_info_iterator &other) const noexcept { return m_iter - other.m_iter; }

			[[nodiscard]] constexpr pointer get() const noexcept;
			[[nodiscard]] constexpr pointer operator->() const noexcept;
			[[nodiscard]] constexpr argument_info operator*() const noexcept;

			[[nodiscard]] constexpr auto operator<=>(const argument_info_iterator &other) const noexcept { return m_iter <=> other.m_iter; }
			[[nodiscard]] constexpr bool operator==(const argument_info_iterator &other) const noexcept { return m_iter == other.m_iter; }

		private:
			iter_t m_iter = {};
			type_domain *m_domain = nullptr;
		};
	}

	/** Handle to reflected constructor argument information. */
	class argument_info
	{
		friend class detail::argument_info_iterator;

	public:
		/** Returns a view of `arguments_info`s for arguments list \a Ts from domain \a domain. */
		template<typename... Ts>
		[[nodiscard]] static constexpr detail::subrange<detail::argument_info_iterator> args_list(type_domain &domain) noexcept;
		/** Returns a view of `arguments_info`s for arguments list \a Ts from the global domain. */
		template<typename... Ts>
		[[nodiscard]] static constexpr detail::subrange<detail::argument_info_iterator> args_list() noexcept;

	private:
		constexpr argument_info(const detail::argument_data *data, type_domain *domain) noexcept : m_data(data), m_domain(domain) {}

	public:
		argument_info() = delete;

		constexpr argument_info(const argument_info &) noexcept = default;
		constexpr argument_info(argument_info &&) noexcept = default;
		constexpr argument_info &operator=(const argument_info &) noexcept = default;
		constexpr argument_info &operator=(argument_info &&) noexcept = default;

		/** Initializes argument info for argument type \a T. */
		template<typename T>
		constexpr argument_info(std::in_place_type_t<T>) noexcept : argument_info(&detail::argument_data::value<T>) {}

		/** Checks if the argument is a reference. */
		[[nodiscard]] constexpr bool is_ref() const noexcept { return !(m_data->flags & detail::IS_VALUE); }
		/** Checks if the argument is const-qualified. */
		[[nodiscard]] constexpr bool is_const() const noexcept { return m_data->flags & detail::IS_CONST; }

		/** Returns type info of the argument's type using type domain \a domain. */
		[[nodiscard]] type_info type(type_domain &domain) const { return {m_data->type, domain}; }
		/** Returns type info of the argument's type. */
		[[nodiscard]] type_info type() const
		{
			if (m_domain != nullptr)
				return type(*type_domain::guard(m_domain).access());
			else
				return type(*type_domain::instance().access());
		}

	private:
		const detail::argument_data *m_data;
		type_domain *m_domain;
	};

	class detail::argument_info_iterator::pointer
	{
		friend class argument_info_iterator;

	public:
		using element_type = argument_info;

	private:
		constexpr pointer(argument_info &&info) noexcept : m_info(std::forward<argument_info>(info)) {}

	public:
		pointer() = delete;

		constexpr pointer(const pointer &) noexcept = default;
		constexpr pointer(pointer &&) noexcept = default;
		constexpr pointer &operator=(const pointer &) noexcept = default;
		constexpr pointer &operator=(pointer &&) noexcept = default;

		[[nodiscard]] constexpr argument_info operator*() noexcept { return m_info; }
		[[nodiscard]] constexpr const argument_info *operator->() noexcept { return &m_info; }

	private:
		argument_info m_info;
	};

	constexpr typename detail::argument_info_iterator::pointer detail::argument_info_iterator::get() const noexcept { return argument_info{*m_iter, m_domain}; }
	constexpr typename detail::argument_info_iterator::pointer detail::argument_info_iterator::operator->() const noexcept { return get(); }
	constexpr argument_info detail::argument_info_iterator::operator*() const noexcept { return *get(); }

	template<typename... Ts>
	constexpr detail::subrange<detail::argument_info_iterator> argument_info::args_list(type_domain &domain) noexcept
	{
		const auto list = detail::argument_data::args_list<Ts...>();
		return {{list.begin(), &domain}, {list.end(), &domain}};
	}
	template<typename... Ts>
	constexpr detail::subrange<detail::argument_info_iterator> argument_info::args_list() noexcept
	{
		return args_list<Ts...>(*type_domain::instance().access());
	}

	/** Exception type thrown when no constructor overload for the specified argument list exists. */
	class REFLEX_PUBLIC type_constructor_error : public std::runtime_error
	{
	public:
		type_constructor_error(const type_constructor_error &) = default;
		type_constructor_error &operator=(const type_constructor_error &) = default;
		type_constructor_error(type_constructor_error &&) = default;
		type_constructor_error &operator=(type_constructor_error &&) = default;

		/** Initializes the constructor error exception from message and a span of arguments. */
		type_constructor_error(const char *msg, detail::subrange<detail::argument_info_iterator> args) : std::runtime_error(msg), m_args(args.begin(), args.end()) {}
		/** @copydoc facet_function_error */
		type_constructor_error(const std::string &msg, detail::subrange<detail::argument_info_iterator> args) : std::runtime_error(msg), m_args(args.begin(), args.end()) {}

#ifdef REFLEX_NO_THREADS
		~type_constructor_error() override = default;
#else
		~type_constructor_error() override;
#endif

		/** Returns span of offending constructor arguments. */
		[[nodiscard]] constexpr detail::subrange<detail::argument_info_iterator> args() const noexcept { return m_args; }

	private:
		detail::subrange<detail::argument_info_iterator> m_args;
	};

	namespace detail
	{
		template<typename T, typename... Ts>
		[[nodiscard]] inline static type_constructor_error make_constructor_error()
		{
			constexpr auto const_name = []<typename U>(std::in_place_type_t<U>)
			{
				constexpr auto name = type_name_v<U>;
				return const_string<name.size()>{name};
			};

			constexpr auto msg = basic_const_string{"`"} + const_name(std::in_place_type<T>) + basic_const_string{"` is not constructible from { "} +
			                     (const_name(std::in_place_type<Ts>) + ... + basic_const_string{" }"});
			return type_constructor_error{{msg.data(), msg.size()}, argument_info::args_list<Ts...>()};
		}

		template<typename T, typename... Ts, typename F, std::size_t... Is>
		[[nodiscard]] inline static T *construct(std::index_sequence<Is...>, F &&f, std::span<any> args)
		{
			return f((std::forward<Ts>(*args[Is].template as<std::remove_reference_t<Ts>>()))...);
		}
		template<typename T, typename... Ts, typename F>
		[[nodiscard]] inline static T *construct(F &&f, std::span<any> args)
		{
			return construct<T, Ts...>(std::make_index_sequence<sizeof...(Ts)>{}, std::forward<F>(f), args);
		}

		template<typename... Ts, typename T, typename F, std::size_t... Is>
		inline static void construct_at(std::index_sequence<Is...>, F &&f, T *ptr, std::span<any> args)
		{
			f(ptr, (std::forward<Ts>(*args[Is].template as<std::remove_reference_t<Ts>>()))...);
		}
		template<typename... Ts, typename T, typename F>
		inline static void construct_at(F &&f, T *ptr, std::span<any> args)
		{
			construct_at<Ts...>(std::make_index_sequence<sizeof...(Ts)>{}, std::forward<F>(f), ptr, args);
		}

		template<typename T, typename PC, typename AC, typename... Ts>
		class type_ctor final : public type_ctor<>
		{
			template<typename C0, typename C1>
			constexpr type_ctor(std::span<const argument_data *const> args, C0 &&ac, C1 &&pc) : type_ctor<>{args}, m_allocating(std::forward<C0>(ac)), m_in_place(std::forward<C1>(pc)) {}

		public:
			[[nodiscard]] static constexpr auto *bind(auto &&pc, auto &&ac) { return new type_ctor{argument_data::args_list<Ts...>(), std::forward<decltype(ac)>(ac), std::forward<decltype(pc)>(pc)}; }

		private:
			static void assert_args(std::span<any> args)
			{
				if (!argument_data::verify(argument_data::args_list<Ts...>(), args))
					[[unlikely]] throw make_constructor_error<T, Ts...>();
			}

		public:
			~type_ctor() override = default;

			void construct_in_place(void *dst, std::span<any> args) override
			{
				assert_args(args);
				construct_at<Ts...>(m_in_place, static_cast<T *>(dst), args);
			}
			[[nodiscard]] void *construct_allocating(std::span<any> args) override
			{
				assert_args(args);
				return construct<T, Ts...>(m_allocating, args);
			}

		private:
			AC m_allocating;
			PC m_in_place;
		};
	}
}