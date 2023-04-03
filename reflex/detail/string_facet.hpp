/*
 * Created by switchblade on 2023-03-25.
 */

#pragma once

#include "range_facet.hpp"

namespace reflex::facets
{
	namespace detail
	{
		template<typename C>
		struct string_vtable
		{
			bool (*empty)(const any &);
			std::size_t (*size)(const any &);

			const C *(*data)(const any &);
			const C *(*c_str)(const any &);
		};
	}

	/** Facet type implementing a generic interface to a string-like type with character type \a C. */
	template<typename C>
	class basic_string : public facet<detail::string_vtable<C>>
	{
		using base_t = facet<detail::string_vtable<C>>;

	public:
		using base_t::base_t;
		using base_t::operator=;

		/** Checks if the underlying string is empty. */
		[[nodiscard]] bool empty() const { return base_t::vtable()->empty(base_t::instance()); }
		/** Returns size of the underlying string. */
		[[nodiscard]] std::size_t size() const { return base_t::vtable()->size(base_t::instance()); }

		/** Returns pointer to the underlying characters of the string. */
		[[nodiscard]] const C *data() const { return base_t::vtable()->data(base_t::instance()); }
		/** Returns pointer to a null-terminated array of characters of the string, or `nullptr` if
		 * the underlying string does not provide a C-style string interface. */
		[[nodiscard]] const C *c_str() const
		{
			auto *func = base_t::vtable()->c_str;
			return func ? func(base_t::instance()) : nullptr;
		}

		/** Casts the underlying string to a string view. */
		[[nodiscard]] operator std::basic_string_view<C>() const { return {data(), size()}; }
	};

	/** Alias for string facet with `char` character type. */
	using string = basic_string<char>;
	/** Alias for string facet with `wchar_t` character type. */
	using wstring = basic_string<wchar_t>;

	template<typename C, std::ranges::contiguous_range T> requires std::same_as<std::ranges::range_value_t<T>, C>
	struct impl_facet<basic_string<C>, T>
	{
	private:
		[[nodiscard]] constexpr static detail::string_vtable<C> make_vtable()
		{
			detail::string_vtable<C> result = {};

			result.empty = +[](const any &str) { return std::ranges::empty(str.as<T>()); };
			result.size = +[](const any &str) { return static_cast<std::size_t>(std::ranges::size(str.as<T>())); };

			result.data = +[](const any &str) { return std::ranges::cdata(str.as<T>()); };

			if constexpr (requires(const T &str) {{ str.c_str() } -> std::same_as<const C *>; })
				result.c_str = +[](const any &str) { return str.as<T>().c_str(); };
			else
				result.c_str = nullptr;

			return result;
		}

	public:
		constexpr static detail::string_vtable<C> value = make_vtable();
	};
}

/** Type initializer overload for STL strings. */
template<typename C, typename T, typename A>
struct reflex::type_init<std::basic_string<C, T, A>>
{
	void operator()(reflex::type_factory<std::basic_string<C, T, A>> f)
	{
		f.template implement_facet<reflex::facets::range>();
		f.template implement_facet<reflex::facets::basic_string<C>>();

		f.template make_constructible<const C *, std::size_t>();
		f.template make_constructible<const C *>();

		f.template make_constructible<std::string_view>();
		f.template make_convertible<std::string_view>();
		f.template make_comparable<std::string_view>();
	}
};
/** Type initializer overload for STL string views. */
template<typename C, typename T>
struct reflex::type_init<std::basic_string_view<C, T>>
{
	void operator()(reflex::type_factory<std::basic_string_view<C, T>> f)
	{
		f.template implement_facet<reflex::facets::range>();
		f.template implement_facet<reflex::facets::basic_string<C>>();

		f.template make_constructible<const C *, std::size_t>();
		f.template make_constructible<const C *>();
	}
};