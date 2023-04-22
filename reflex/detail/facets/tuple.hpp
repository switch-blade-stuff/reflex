/*
 * Created by switchblade on 2023-03-25.
 */

#pragma once

#include "../facet.hpp"

namespace reflex::facets
{
	namespace detail
	{
		struct tuple_vtable
		{
			std::size_t size;
			type_info (*tuple_element)(std::size_t);

			any (*get)(any &, std::size_t);
			any (*get_const)(const any &, std::size_t);
		};
	}

	/** Facet type implementing a generic interface to a tuple-like type. */
	class tuple : public facet<detail::tuple_vtable>
	{
		using base_t = facet<detail::tuple_vtable>;

	public:
		using base_t::facet;
		using base_t::operator=;

		/** Returns size of the underlying tuple object as if via `std::tuple_size`. */
		[[nodiscard]] std::size_t size() const noexcept { return base_t::vtable()->size; }
		/** Returns type of the `n`th element of the tuple. If \a n is greater than tuple size, returns invalid type info. */
		[[nodiscard]] type_info tuple_element(std::size_t n) const { return base_t::checked_invoke<&vtable_type::tuple_element, "type_info tuple_element(size_type) const">(n); }

		/** Returns `n`th element of the tuple. If \a n is greater than tuple size, returns an empty `any`. */
		[[nodiscard]] any get(std::size_t n) { return base_t::checked_invoke<&vtable_type::get, "reference get(size_type)">(instance(), n); }
		/** @copydoc get */
		[[nodiscard]] any get(std::size_t n) const { return base_t::checked_invoke<&vtable_type::get_const, "const_reference get(size_type) const">(instance(), n); }

		/** Checks if the tuple has size of 2. */
		[[nodiscard]] bool is_pair() const noexcept { return size() == 2; }

		/** Returns type of the first element of the tuple. */
		[[nodiscard]] type_info first_type() const { return tuple_element(0); }
		/** Returns type of the second element of the tuple. */
		[[nodiscard]] type_info second_type() const { return tuple_element(1); }

		/** Returns the first element of the tuple. */
		[[nodiscard]] any first() { return get(0); }
		/** @copydoc first */
		[[nodiscard]] any first() const { return get(0); }

		/** Returns the second element of the tuple. */
		[[nodiscard]] any second() { return get(1); }
		/** @copydoc second */
		[[nodiscard]] any second() const { return get(1); }
	};

	template<reflex::tuple_like T>
	struct impl_facet<tuple, T>
	{
	private:
		template<std::size_t I>
		[[nodiscard]] constexpr static type_info element_type(std::size_t n)
		{
			if constexpr (I < std::tuple_size_v<T>)
			{
				if (n == I)
					return type_info::get<std::tuple_element_t<I, T>>();
				else
					return element_type<I + 1>(n);
			}
			return type_info{};
		}
		template<std::size_t I = 0>
		[[nodiscard]] static any element_get(auto &tuple, std::size_t n)
		{
			if constexpr (I < std::tuple_size_v<T>)
			{
				if (n == I)
					return forward_any(get<I>(tuple));
				else
					return element_get<I + 1>(tuple, n);
			}
			return any{};
		}

		[[nodiscard]] constexpr static detail::tuple_vtable make_vtable()
		{
			detail::tuple_vtable result = {};

			result.size = std::tuple_size_v<T>;
			result.tuple_element = element_type<0>;

			result.get = +[](any &tuple, std::size_t n) { return element_get(tuple.as<T>(), n); };
			result.get_const = +[](const any &tuple, std::size_t n) { return element_get(tuple.as<T>(), n); };

			return result;
		}

	public:
		constexpr static detail::tuple_vtable value = make_vtable();
	};
}

/** Type initializer overload for tuple-like types. */
template<reflex::tuple_like T>
struct reflex::type_init<T> { void operator()(reflex::type_factory<T> f) { f.template implement_facet<reflex::facets::tuple>(); }};

/** Type initializer overload for both range-like & tuple-like types. */
template<typename R> requires reflex::tuple_like<R> && std::ranges::input_range<R>
struct reflex::type_init<R>
{
	void operator()(reflex::type_factory<R> f)
	{
		f.template implement_facet<reflex::facets::range>();
		f.template implement_facet<reflex::facets::tuple>();
	}
};