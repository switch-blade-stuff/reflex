/*
 * Created by switchblade on 2023-04-01.
 */

#pragma once

#include "database.hpp"

namespace reflex
{
	template<typename... Filters>
	class type_query;

	namespace detail
	{
		struct query_base
		{
			using filter_func = bool (*)(const void *, type_info);

			constexpr query_base() noexcept = default;
			constexpr explicit query_base(database_impl *db) noexcept : m_db(db) {}

			database_impl *m_db;
		};

		template<typename... Filters, typename F>
		[[nodiscard]] inline static type_query<Filters..., F> push_filter(type_query<Filters...> &&, F &&);
	}

	/** Utility class used to preform queries of reflected type info. */
	template<typename... Filters>
	class type_query : detail::query_base, std::tuple<Filters...>
	{
		friend class type_info;
		template<typename...>
		friend class type_query;
		template<typename... Fs, typename F>
		friend inline type_query<Fs..., F> detail::push_filter(type_query<Fs...> &&, F &&);

		template<typename F>
		static bool proxy(const void *ptr, type_info type) { return std::get<F>(*static_cast<const type_query *>(ptr))(type); }

		constexpr type_query() noexcept = default;
		template<typename F>
		constexpr type_query(type_query<> &&other, F &&f) : std::tuple<Filters...>{std::forward<F>(f)} { m_db = other.m_db; }
		template<typename... Other, typename F>
		constexpr type_query(type_query<Other...> &&other, F &&f) : std::tuple<Filters...>{std::tuple_cat(std::move(other.filters()), std::forward<F>(f))} { m_db = other.m_db; }

		constexpr explicit type_query(detail::database_impl *db) noexcept requires(sizeof...(Filters) == 0) : detail::query_base(db) {}

	public:
		/** Filters the query for types that satisfy predicate \a pred. */
		template<typename P>
		inline auto satisfies(P &&pred) && requires std::is_invocable_r_v<bool, P, type_info>;

		/** Filters the query for enum types. */
		inline auto is_enum() &&;
		/** Filters the query for class types. */
		inline auto is_class() &&;
		/** Filters the query for abstract types. */
		inline auto is_abstract() &&;
		/** Filters the query for pointer types. */
		inline auto is_pointer() &&;
		/** Filters the query for integral types. */
		inline auto is_integral() &&;
		/** Filters the query for signed integral types. */
		inline auto is_signed_integral() &&;
		/** Filters the query for unsigned integral types. */
		inline auto is_unsigned_integral() &&;
		/** Filters the query for arithmetic types. */
		inline auto is_arithmetic() &&;
		/** Filters the query for array types. */
		inline auto is_array() &&;

		/** Filters the query for types that have an attribute of type \a T. */
		template<typename T>
		inline auto has_attribute() &&;
		/** Filters the query for types that have an attribute of type \a type. */
		inline auto has_attribute(type_info type) &&;

		/** Filters the query for types that have an enumeration with value \a value. */
		template<typename T>
		inline auto has_enumeration(T &&value) &&;
		/** Filters the query for types that have an enumeration with name \a name. */
		inline auto has_enumeration(std::string_view name) &&;

		/** Filters the query for types that implement facet (or facet group) of type \a F. */
		template<typename F>
		inline auto implements_facet() &&;
		/** Filters the query for types that implement facet of type \a type. */
		inline auto implements_facet(type_info type) &&;

		/** Filters the query for types that inherit from type \a T. */
		template<typename T>
		inline auto inherits_from() &&;
		/** Filters the query for types that inherit from type \a type. */
		inline auto inherits_from(type_info type) &&;

		/** Filters the query for types constructible from arguments \a Args. */
		template<typename... Args>
		inline auto constructible_from() &&;
		/** Filters the query for types constructible from arguments \a args. */
		inline auto constructible_from(std::span<any> args) &&;
		/** @copydoc constructible_from */
		inline auto constructible_from(argument_list args) &&;

		/** Filters the query for types convertible to type \a T. */
		template<typename T>
		inline auto convertible_to() &&;
		/** Filters the query for types convertible to type \a type. */
		inline auto convertible_to(type_info type) &&;

		/** Filters the query for types compatible with type \a T. */
		template<typename T>
		inline auto compatible_with() &&;
		/** Filters the query for types compatible with type \a type. */
		inline auto compatible_with(type_info type) &&;

		/** Filters the query for types directly (without conversion) comparable (using any comparison operator) with type \a T. */
		template<typename T>
		inline auto comparable_with() &&;
		/** Filters the query for types directly (without conversion) comparable (using any comparison operator) with type \a type. */
		inline auto comparable_with(type_info type) &&;

		/** Filters the query for types directly (without conversion) comparable with type \a T using `operator==` and `operator!=`. */
		template<typename T>
		inline auto eq_comparable_with() &&;
		/** Filters the query for types directly (without conversion) comparable with type \a type using `operator==` and `operator!=`. */
		inline auto eq_comparable_with(type_info type) &&;

		/** Filters the query for types directly (without conversion) comparable with type \a T using `operator>=`. */
		template<typename T>
		inline auto ge_comparable_with() &&;
		/** Filters the query for types directly (without conversion) comparable with type \a type using `operator>=`. */
		inline auto ge_comparable_with(type_info type) &&;

		/** Filters the query for types directly (without conversion) comparable with type \a T using `operator<=`. */
		template<typename T>
		inline auto le_comparable_with() &&;
		/** Filters the query for types directly (without conversion) comparable with type \a type using `operator<=`. */
		inline auto le_comparable_with(type_info type) &&;

		/** Filters the query for types directly (without conversion) comparable with type \a T using `operator>`. */
		template<typename T>
		inline auto gt_comparable_with() &&;
		/** Filters the query for types directly (without conversion) comparable with type \a type using `operator>`. */
		inline auto gt_comparable_with(type_info type) &&;

		/** Filters the query for types directly (without conversion) comparable with type \a T using `operator<`. */
		template<typename T>
		inline auto lt_comparable_with() &&;
		/** Filters the query for types directly (without conversion) comparable with type \a type using `operator<`. */
		inline auto lt_comparable_with(type_info type) &&;

		/** Returns a set of `type_info`s matched by the query. */
		[[nodiscard]] detail::type_set types() const { return filter(std::array{proxy<Filters>...}); }

	private:
		[[nodiscard]] constexpr std::tuple<Filters...> &filters() noexcept { return *this; }
		[[nodiscard]] constexpr const std::tuple<Filters...> &filters() const noexcept { return *this; }

		[[nodiscard]] detail::type_set filter(std::span<const filter_func> funcs) const
		{
			return reinterpret_cast<const type_query<> *>(this)->filter(funcs);
		}
	};

	template<>
	REFLEX_PUBLIC detail::type_set type_query<>::types() const;
	template<>
	REFLEX_PUBLIC detail::type_set type_query<>::filter(std::span<const filter_func>) const;

	type_query<> type_info::query() { return type_query<>{detail::database_impl::instance()}; }

	template<typename... Filters, typename F>
	[[nodiscard]] inline static type_query<Filters..., F> detail::push_filter(type_query<Filters...> &&other, F &&filter)
	{
		return {std::move(other), std::forward<F>(filter)};
	}

	template<typename... Filters>
	template<typename P>
	auto type_query<Filters...>::satisfies(P &&pred) && requires std::is_invocable_r_v<bool, P, type_info>
	{
		return detail::push_filter(std::move(*this), std::forward<P>(pred));
	}

	template<typename... Filters>
	auto type_query<Filters...>::is_enum() && { return std::move(*this).satisfies([](auto t) { return t.is_enum(); }); }
	template<typename... Filters>
	auto type_query<Filters...>::is_class() && { return std::move(*this).satisfies([](auto t) { return t.is_class(); }); }
	template<typename... Filters>
	auto type_query<Filters...>::is_abstract() && { return std::move(*this).satisfies([](auto t) { return t.is_abstract(); }); }
	template<typename... Filters>
	auto type_query<Filters...>::is_pointer() && { return std::move(*this).satisfies([](auto t) { return t.is_pointer(); }); }
	template<typename... Filters>
	auto type_query<Filters...>::is_integral() && { return std::move(*this).satisfies([](auto t) { return t.is_integral(); }); }
	template<typename... Filters>
	auto type_query<Filters...>::is_signed_integral() && { return std::move(*this).satisfies([](auto t) { return t.is_signed_integral(); }); }
	template<typename... Filters>
	auto type_query<Filters...>::is_unsigned_integral() && { return std::move(*this).satisfies([](auto t) { return t.is_unsigned_integral(); }); }
	template<typename... Filters>
	auto type_query<Filters...>::is_arithmetic() && { return std::move(*this).satisfies([](auto t) { return t.is_arithmetic(); }); }
	template<typename... Filters>
	auto type_query<Filters...>::is_array() && { return std::move(*this).satisfies([](auto t) { return t.is_array(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::has_attribute(type_info type) && { return std::move(*this).satisfies([=](auto t) { return t.has_attribute(type); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::has_attribute() && { return std::move(*this).satisfies([](auto t) { return t.template has_attribute<T>(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::has_enumeration(std::string_view name) && { return std::move(*this).satisfies([=](auto t) { return t.has_enumeration(name); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::has_enumeration(T &&value) && { return std::move(*this).satisfies([v = forward_any(value)](auto t) { return t.has_enumeration(v); }); }

	template<typename... Filters>
	auto type_query<Filters...>::implements_facet(type_info type) && { return std::move(*this).satisfies([=](auto t) { return t.implements_facet(type); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::implements_facet() && { return std::move(*this).satisfies([](auto t) { return t.template implements_facet<T>(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::inherits_from(type_info type) && { return std::move(*this).satisfies([=](auto t) { return t.inherits_from(type); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::inherits_from() && { return std::move(*this).satisfies([](auto t) { return t.template inherits_from<T>(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::constructible_from(argument_list args) && { return std::move(*this).satisfies([=](auto t) { return t.constructible_from(args); }); }
	template<typename... Filters>
	auto type_query<Filters...>::constructible_from(std::span<any> args) && { return std::move(*this).satisfies([=](auto t) { return t.constructible_from(args); }); }
	template<typename... Filters>
	template<typename... Args>
	auto type_query<Filters...>::constructible_from() && { return std::move(*this).satisfies([](auto t) { return t.template constructible_from<Args...>(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::convertible_to(type_info type) && { return std::move(*this).satisfies([=](auto t) { return t.convertible_to(type); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::convertible_to() && { return std::move(*this).satisfies([](auto t) { return t.template convertible_to<T>(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::compatible_with(type_info type) && { return std::move(*this).satisfies([=](auto t) { return t.compatible_with(type); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::compatible_with() && { return std::move(*this).satisfies([](auto t) { return t.template compatible_with<T>(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::comparable_with(type_info type) && { return std::move(*this).satisfies([=](auto t) { return t.comparable_with(type); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::comparable_with() && { return std::move(*this).satisfies([](auto t) { return t.template comparable_with<T>(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::eq_comparable_with(type_info type) && { return std::move(*this).satisfies([=](auto t) { return t.eq_comparable_with(type); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::eq_comparable_with() && { return std::move(*this).satisfies([](auto t) { return t.template eq_comparable_with<T>(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::ge_comparable_with(type_info type) && { return std::move(*this).satisfies([=](auto t) { return t.ge_comparable_with(type); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::ge_comparable_with() && { return std::move(*this).satisfies([](auto t) { return t.template ge_comparable_with<T>(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::le_comparable_with(type_info type) && { return std::move(*this).satisfies([=](auto t) { return t.le_comparable_with(type); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::le_comparable_with() && { return std::move(*this).satisfies([](auto t) { return t.template le_comparable_with<T>(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::gt_comparable_with(type_info type) && { return std::move(*this).satisfies([=](auto t) { return t.gt_comparable_with(type); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::gt_comparable_with() && { return std::move(*this).satisfies([](auto t) { return t.template gt_comparable_with<T>(); }); }

	template<typename... Filters>
	auto type_query<Filters...>::lt_comparable_with(type_info type) && { return std::move(*this).satisfies([=](auto t) { return t.lt_comparable_with(type); }); }
	template<typename... Filters>
	template<typename T>
	auto type_query<Filters...>::lt_comparable_with() && { return std::move(*this).satisfies([](auto t) { return t.template lt_comparable_with<T>(); }); }
}