/*
 * Created by switchblade on 2023-04-03.
 */

#pragma once

#include "../facet.hpp"

namespace reflex::facets
{
	namespace detail
	{
		struct pointer_vtable
		{
			type_info (*element_type)() = {};

			bool (*empty)(const any &) = {};

			any (*get)(const any &) = {};
			const void *(*data)(const any &) = {};

			any (*deref)(const any &) = {};
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
		[[nodiscard]] bool empty() const { return base_t::checked_invoke<&vtable_type::empty, "bool empty() const">(base_t::instance()); }

		/** Returns the result of `std::to_address` invoked on the underlying pointer. */
		[[nodiscard]] any get() const { return base_t::checked_invoke<&vtable_type::get, "pointer get() const">(base_t::instance()); }
		/** Returns void pointer to the object pointed to by the underlying pointer. */
		[[nodiscard]] const void *data() const { return base_t::checked_invoke<&vtable_type::data, "const void *data() const">(base_t::instance()); }

		/** Dereferences the underlying pointer. If the pointer is not dereferenceable, returns an empty `any`. */
		[[nodiscard]] any deref() const { return base_t::checked_invoke<&vtable_type::deref, "reference deref() const">(base_t::instance()); }
	};

	template<pointer_like P>
	struct impl_facet<pointer, P>
	{
	private:
		[[nodiscard]] constexpr static detail::pointer_vtable make_vtable()
		{
			detail::pointer_vtable result = {};
			result.element_type = type_info::get<typename std::pointer_traits<P>::element_type>;
			result.data = +[](const any &ptr) -> const void * { return std::to_address(ptr.as<P>()); };
			result.get = +[](const any &ptr) { return forward_any(std::to_address(ptr.as<P>())); };

			result.empty = +[](const any &ptr) -> bool
			{
				const auto &obj = ptr.as<P>();
				if constexpr (requires {{ obj.empty() } -> std::convertible_to<bool>; })
					return static_cast<bool>(obj.empty());
				else
					return !std::to_address(obj);
			};
			result.deref = +[](const any &ptr)
			{
				if constexpr (requires (const P &p) { *p; })
					return forward_any(*ptr.as<P>());
				else
					return any{};
			};

			return result;
		}

	public:
		constexpr static detail::pointer_vtable value = make_vtable();
	};
}

/** Type initializer overload for pointer-like types. */
template<reflex::pointer_like T>
struct reflex::type_init<T> { void operator()(reflex::type_factory<T> f) { f.template implement_facet<reflex::facets::pointer>(); }};