/*
 * Created by switchblade on 2023-04-03.
 */

#pragma once

#include "facet.hpp"

namespace reflex::facets
{
	namespace detail
	{
		struct pointer_vtable
		{
			type_info (*element_type)();

			bool (*empty)(const any &);
			const void *(*data)(const any &);
			any (*deref)(const any &);
		};
	}

	/** Facet type implementing a generic pointer-like type. */
	class pointer : public facet<detail::pointer_vtable>
	{
		using base_t = facet<detail::pointer_vtable>;

	public:
		using base_t::base_t;
		using base_t::operator=;

		/** Checks if the underlying pointer is empty. */
		[[nodiscard]] bool empty() const { return base_t::vtable()->empty(base_t::instance()); }
		/** Returns void pointer to the object pointed to by the underlying pointer. */
		[[nodiscard]] const void *data() const { return base_t::vtable()->data(base_t::instance()); }
		/** Dereferences the underlying pointer. */
		[[nodiscard]] any deref() const { return base_t::vtable()->deref(base_t::instance()); }
		/** @copydoc deref */
		[[nodiscard]] any operator*() const { return deref(); }
	};

	template<pointer_like P>
	struct impl_facet<pointer, P>
	{
	private:
		[[nodiscard]] constexpr static detail::pointer_vtable make_vtable()
		{
			detail::pointer_vtable result = {};
			result.element_type = type_info::get<typename std::pointer_traits<P>::element_type>;

			result.empty = +[](const any &ptr) -> bool
			{
				const auto &obj = ptr.as<P>();
				if constexpr (requires { { obj.empty() } -> std::convertible_to<bool>; })
					return static_cast<bool>(obj.empty());
				else
					return !std::to_address(obj);
			};
			result.data = +[](const any &ptr) -> const void * { return std::to_address(ptr.as<P>()); };
			result.deref = +[](const any &ptr) { return forward_any(*ptr.as<P>()); };

			return result;
		}

	public:
		constexpr static detail::pointer_vtable value = make_vtable();
	};
}

/** Type initializer overload for pointer-like types. */
template<reflex::pointer_like T>
struct reflex::type_init<T> { void operator()(reflex::type_factory<T> f) { f.template implement_facet<reflex::facets::pointer>(); }};