/*
 * Created by switchblade on 2023-03-25.
 */

#pragma once

#include "facet.hpp"
#include "any.hpp"

namespace reflex
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
	class tuple_facet : public facet<detail::tuple_vtable>
	{
		using base_t = facet<detail::tuple_vtable>;

	public:
		/** Returns size of the underlying tuple object as if via `std::tuple_size`. */
		[[nodiscard]] std::size_t size() const noexcept { return base_t::vtable()->size; }
		/** Returns type of the `n`th element of the tuple. If \a n is greater than tuple size, returns invalid type info. */
		[[nodiscard]] type_info tuple_element(std::size_t n) const { return base_t::vtable()->tuple_element(n); }

		/** Returns `n`th element of the tuple. If \a n is greater than tuple size, returns an empty `any`. */
		[[nodiscard]] any get(std::size_t n) { return base_t::vtable()->get(instance(), n); }
		/** @copydoc get */
		[[nodiscard]] any get(std::size_t n) const { return base_t::vtable()->get_const(instance(), n); }
	};

	template<typename T>
	struct impl_facet<tuple_facet, T>
	{
	private:
		template<std::size_t I>
		[[nodiscard]] constexpr static type_info element_type(std::size_t n)
		{
			if (n == I)
				return type_info::get<std::tuple_element_t<I, T>>();
			else if constexpr (I < std::tuple_size_v<T>)
				return element_type<I + 1>(n);
			else
				return type_info{};
		}
		template<std::size_t I = 0>
		[[nodiscard]] static any element_get(auto &tuple, std::size_t n)
		{
			if (n == I)
			{
				using std::get;
				return forward_any(get<I>(tuple));
			}
			else if constexpr (I < std::tuple_size_v<T>)
				return element_get<I + 1>(tuple, n);
			else
				return any{};
		}

		[[nodiscard]] constexpr static detail::tuple_vtable make_vtable()
		{
			detail::tuple_vtable result = {};

			result.size = std::tuple_size_v<T>;
			result.tuple_element = element_type<0>;

			result.get = +[](any &tuple, std::size_t n)
			{
				auto &tuple_ref = *tuple.as<T>();
				return element_get(tuple_ref, n);
			};
			result.get_const = +[](const any &tuple, std::size_t n)
			{
				auto &tuple_ref = *tuple.as<T>();
				return element_get(tuple_ref, n);
			};

			return result;
		}

	public:
		constexpr static detail::tuple_vtable value = make_vtable();
	};
}