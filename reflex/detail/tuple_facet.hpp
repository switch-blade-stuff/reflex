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