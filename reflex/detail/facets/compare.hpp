/*
 * Created by switchblade on 2023-04-21.
 */

#pragma once

#include "../facet.hpp"

namespace reflex
{
	namespace facets
	{
		class compare;

		namespace detail
		{
			struct cmp_vtable
			{
				bool (*cmp_eq)(const any &, const any &) = {};
				bool (*cmp_ne)(const any &, const any &) = {};
				bool (*cmp_ge)(const any &, const any &) = {};
				bool (*cmp_le)(const any &, const any &) = {};
				bool (*cmp_gt)(const any &, const any &) = {};
				bool (*cmp_lt)(const any &, const any &) = {};
			};
		}

		/** Facet type implementing a comparable type. */
		class compare : public facet<detail::cmp_vtable>
		{
			using base_t = facet<detail::cmp_vtable>;

		public:
			using base_t::base_t;
			using base_t::operator=;

			/** Checks if the underlying object is equal to \a other. */
			[[nodiscard]] bool cmp_eq(const any &other) const { return base_t::checked_invoke<&vtable_type::cmp_eq, "bool cmp_eq(const T &) const">(base_t::instance(), other); }
			/** Checks if the underlying object is not equal to \a other. */
			[[nodiscard]] bool cmp_ne(const any &other) const { return base_t::checked_invoke<&vtable_type::cmp_ne, "bool cmp_ne(const T &) const">(base_t::instance(), other); }
			/** Checks if the underlying object is greater than or equal to \a other. */
			[[nodiscard]] bool cmp_ge(const any &other) const { return base_t::checked_invoke<&vtable_type::cmp_ge, "bool cmp_ge(const T &) const">(base_t::instance(), other); }
			/** Checks if the underlying object is less than or equal to \a other. */
			[[nodiscard]] bool cmp_le(const any &other) const { return base_t::checked_invoke<&vtable_type::cmp_le, "bool cmp_le(const T &) const">(base_t::instance(), other); }
			/** Checks if the underlying object is greater than \a other. */
			[[nodiscard]] bool cmp_gt(const any &other) const { return base_t::checked_invoke<&vtable_type::cmp_gt, "bool cmp_gt(const T &) const">(base_t::instance(), other); }
			/** Checks if the underlying object is less than or \a other. */
			[[nodiscard]] bool cmp_lt(const any &other) const { return base_t::checked_invoke<&vtable_type::cmp_lt, "bool cmp_lt(const T &) const">(base_t::instance(), other); }
		};

		template<typename T>
		struct impl_facet<compare, T>
		{
		private:
#ifdef _MSC_VER
			/* Allow signed/unsigned comparison. */
#pragma warning(push)
#pragma warning(disable : 4018)
#endif
			[[nodiscard]] constexpr static detail::cmp_vtable make_vtable()
			{
				detail::cmp_vtable result = {};
				result.cmp_eq = [](const any &a, const any &b)
				{
					if constexpr (requires(const T &l, const T &r) { l == r; })
					{
						auto *a_ptr = a.try_as<const T>();
						auto *b_ptr = b.try_as<const T>();
						return a_ptr && b_ptr && *a_ptr == *b_ptr;
					}
					return false;
				};
				result.cmp_ne = [](const any &a, const any &b)
				{
					if constexpr (requires(const T &l, const T &r) { l != r; })
					{
						auto *a_ptr = a.try_as<const T>();
						auto *b_ptr = b.try_as<const T>();
						return a_ptr && b_ptr && *a_ptr != *b_ptr;
					}
					return false;
				};
				result.cmp_le = [](const any &a, const any &b)
				{
					if constexpr (requires(const T &l, const T &r) { l <= r; })
					{
						auto *a_ptr = a.try_as<const T>();
						auto *b_ptr = b.try_as<const T>();
						return a_ptr && b_ptr && *a_ptr <= *b_ptr;
					}
					return false;
				};
				result.cmp_ge = [](const any &a, const any &b)
				{
					if constexpr (requires(const T &l, const T &r) { l >= r; })
					{
						auto *a_ptr = a.try_as<const T>();
						auto *b_ptr = b.try_as<const T>();
						return a_ptr && b_ptr && *a_ptr >= *b_ptr;
					}
					return false;
				};
				result.cmp_lt = [](const any &a, const any &b)
				{
					if constexpr (requires(const T &l, const T &r) { l < r; })
					{
						auto *a_ptr = a.try_as<const T>();
						auto *b_ptr = b.try_as<const T>();
						return a_ptr && b_ptr && *a_ptr < *b_ptr;
					}
					return false;
				};
				result.cmp_gt = [](const any &a, const any &b)
				{
					if constexpr (requires(const T &l, const T &r) { l > r; })
					{
						auto *a_ptr = a.try_as<const T>();
						auto *b_ptr = b.try_as<const T>();
						return a_ptr && b_ptr && *a_ptr > *b_ptr;
					}
					return false;
				};
				return result;
			}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		public:
			constexpr static detail::cmp_vtable value = make_vtable();
		};
	}

	template<typename T>
	void detail::type_data::init_cmp_vtable(type_data &data)
	{
		using namespace facets;
		data.vtabs.emplace_or_replace(type_name_v<compare::vtable_type>, &impl_facet_v<compare, T>);
	}
}
