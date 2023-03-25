/*
 * Created by switchblade on 2023-03-24.
 */

#pragma once

#include "facet.hpp"
#include "any.hpp"

namespace reflex
{
	namespace detail
	{
		struct range_vtable;

		class any_iterator
		{
		public:
			using value_type = any;
			using difference_type = std::ptrdiff_t;

		private:
			template<basic_const_string Name>
			static void assert_vtable(auto *ptr)
			{
				constexpr auto msg = basic_const_string{"Failed to invoke facet function `"} + Name + basic_const_string{"`"};
				if (ptr != nullptr) throw bad_facet_function(msg.data(), Name);
			}

			any_iterator(const range_vtable *vtable, const any &value) : m_vtable(vtable), m_value(value) {}
			any_iterator(const range_vtable *vtable, any &&value) : m_vtable(vtable), m_value(std::forward<any>(value)) {}

		public:
			any_iterator() noexcept = default;

			any_iterator &operator++();
			any_iterator &operator--();
			[[nodiscard]] any_iterator operator++(int);
			[[nodiscard]] any_iterator operator--(int);

			any_iterator &operator+=(difference_type n);
			any_iterator &operator-=(difference_type n);

			[[nodiscard]] any_iterator operator+(difference_type n) const;
			[[nodiscard]] any_iterator operator-(difference_type n) const;
			[[nodiscard]] difference_type operator-(const any_iterator &other) const;

			[[nodiscard]] value_type operator*() const;

			[[nodiscard]] bool operator==(const any_iterator &other) const { return m_value == other.m_value; }
			[[nodiscard]] bool operator!=(const any_iterator &other) const { return m_value != other.m_value; }
			[[nodiscard]] bool operator>=(const any_iterator &other) const { return m_value >= other.m_value; }
			[[nodiscard]] bool operator<=(const any_iterator &other) const { return m_value <= other.m_value; }
			[[nodiscard]] bool operator>(const any_iterator &other) const { return m_value > other.m_value; }
			[[nodiscard]] bool operator<(const any_iterator &other) const { return m_value < other.m_value; }

		private:
			const range_vtable *m_vtable = nullptr;
			any m_value;
		};

		struct range_vtable
		{
			/* Iterator-related functions. */
			void (*iter_pre_inc)(any &);
			void (*iter_pre_dec)(any &);
			any (*iter_post_inc)(const any &);
			any (*iter_post_dec)(const any &);
			void (*iter_eq_add)(any &, std::ptrdiff_t);
			void (*iter_eq_sub)(any &, std::ptrdiff_t);
			any (*iter_add)(const any &, std::ptrdiff_t);
			any (*iter_sub)(const any &, std::ptrdiff_t);
			std::ptrdiff_t (*iter_diff)(const any &, const any &);
			any (*iter_deref)(const any &);
		};

		any_iterator &any_iterator::operator++()
		{
			assert_vtable<"iterator &iterator::operator++()">(m_vtable->iter_pre_inc);
			m_vtable->iter_pre_inc(m_value);
			return *this;
		}
		any_iterator &any_iterator::operator--()
		{
			assert_vtable<"iterator &iterator::operator--()">(m_vtable->iter_pre_dec);
			m_vtable->iter_pre_dec(m_value);
			return *this;
		}
		any_iterator any_iterator::operator++(int)
		{
			assert_vtable<"iterator iterator::operator++(int)">(m_vtable->iter_post_inc);
			return {m_vtable, m_vtable->iter_post_inc(m_value)};
		}
		any_iterator any_iterator::operator--(int)
		{
			assert_vtable<"iterator iterator::operator--(int)">(m_vtable->iter_post_dec);
			return {m_vtable, m_vtable->iter_post_dec(m_value)};
		}

		any_iterator &any_iterator::operator+=(difference_type n)
		{
			assert_vtable<"iterator &iterator::operator+=(difference_type)">(m_vtable->iter_eq_add);
			m_vtable->iter_eq_add(m_value, n);
			return *this;
		}
		any_iterator &any_iterator::operator-=(difference_type n)
		{
			assert_vtable<"iterator &iterator::operator-=(difference_type)">(m_vtable->iter_eq_sub);
			m_vtable->iter_eq_sub(m_value, n);
			return *this;
		}

		any_iterator any_iterator::operator+(difference_type n) const
		{
			assert_vtable<"iterator iterator::operator+(difference_type) const">(m_vtable->iter_add);
			return {m_vtable, m_vtable->iter_add(m_value, n)};
		}
		any_iterator any_iterator::operator-(difference_type n) const
		{
			assert_vtable<"iterator iterator::operator-(difference_type) const">(m_vtable->iter_sub);
			return {m_vtable, m_vtable->iter_sub(m_value, n)};
		}
		typename any_iterator::difference_type any_iterator::operator-(const any_iterator &other) const
		{
			assert_vtable<"difference_type iterator::operator-(const iterator &) const">(m_vtable->iter_diff);
			return m_vtable->iter_diff(m_value, other.m_value);
		}

		any any_iterator::operator*() const
		{
			assert_vtable<"value_type iterator::operator*()">(m_vtable->iter_deref);
			return m_vtable->iter_deref(m_value);
		}
	}

	/** Facet type implementing a generic range. */
	class range_facet : public facet<detail::range_vtable>
	{
	public:
	};

	/** Facet type implementing a generic table-like range. */
	class table_facet : public facet<detail::range_vtable>
	{
	};
}