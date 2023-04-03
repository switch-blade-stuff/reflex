/*
 * Created by switchblade on 2023-03-24.
 */

#pragma once

#include "facet.hpp"

namespace reflex::facets
{
	class range;

	namespace detail
	{
		struct iterator_vtable
		{
			any (*iter_deref)(const any &) = nullptr;

			void (*iter_pre_inc)(any &) = nullptr;
			void (*iter_pre_dec)(any &) = nullptr;
			any (*iter_post_inc)(any &) = nullptr;
			any (*iter_post_dec)(any &) = nullptr;

			void (*iter_eq_add)(any &, std::ptrdiff_t) = nullptr;
			void (*iter_eq_sub)(any &, std::ptrdiff_t) = nullptr;
			any (*iter_add)(const any &, std::ptrdiff_t) = nullptr;
			any (*iter_sub)(const any &, std::ptrdiff_t) = nullptr;

			std::ptrdiff_t (*iter_diff)(const any &, const any &) = nullptr;
		};

		class any_iterator
		{
			friend class reflex::facets::range;

		public:
			using difference_type = std::ptrdiff_t;
			using iterator_category = std::input_iterator_tag;

		private:
			template<basic_const_string Name>
			static void assert_vtable(auto *ptr)
			{
				constexpr auto msg = basic_const_string{"Failed to invoke facet function `"} + Name + basic_const_string{"`"};
				if (ptr != nullptr) throw bad_facet_function(msg.data(), Name);
			}

			any_iterator(const iterator_vtable *vtable, const any &value) : m_vtable(vtable), m_value(value) {}
			any_iterator(const iterator_vtable *vtable, any &&value) : m_vtable(vtable), m_value(std::forward<any>(value)) {}

		public:
			any_iterator() noexcept = default;

			/** Returns reference to the underlying iterator instance. */
			[[nodiscard]] any &instance() { return m_value; }
			/** @copydoc instance */
			[[nodiscard]] const any &instance() const { return m_value; }

			/** Pre-increments the underlying iterator by 1. */
			any_iterator &operator++()
			{
				assert_vtable<"iterator &iterator::operator++()">(m_vtable->iter_pre_inc);
				m_vtable->iter_pre_inc(m_value);
				return *this;
			}
			/** Pre-decrements the underlying iterator by 1.
			 * @throw bad_facet_function If the underlying iterator type is not a bidirectional iterator. */
			any_iterator &operator--()
			{
				assert_vtable<"iterator &iterator::operator--()">(m_vtable->iter_pre_dec);
				m_vtable->iter_pre_dec(m_value);
				return *this;
			}
			/** Increments the underlying iterator by \a n.
			 * @throw bad_facet_function If the underlying iterator type is not a random-access iterator. */
			any_iterator &operator+=(difference_type n)
			{
				assert_vtable<"iterator &iterator::operator+=(difference_type)">(m_vtable->iter_eq_add);
				m_vtable->iter_eq_add(m_value, n);
				return *this;
			}
			/** Decrements the underlying iterator by \a n.
			 * @throw bad_facet_function If the underlying iterator type is not a random-access iterator. */
			any_iterator &operator-=(difference_type n)
			{
				assert_vtable<"iterator &iterator::operator-=(difference_type)">(m_vtable->iter_eq_sub);
				m_vtable->iter_eq_sub(m_value, n);
				return *this;
			}

			/** Post-increments the underlying iterator by 1. */
			any_iterator operator++(int)
			{
				assert_vtable<"iterator iterator::operator++(int)">(m_vtable->iter_post_inc);
				return {m_vtable, m_vtable->iter_post_inc(m_value)};
			}
			/** Post-decrements the underlying iterator by 1.
			 * @throw bad_facet_function If the underlying iterator type is not a bidirectional iterator. */
			any_iterator operator--(int)
			{
				assert_vtable<"iterator iterator::operator--(int)">(m_vtable->iter_post_dec);
				return {m_vtable, m_vtable->iter_post_dec(m_value)};
			}
			/** Returns an iterator located \a n elements after the underlying iterator.
			 * @throw bad_facet_function If the underlying iterator type is not a random-access iterator. */
			[[nodiscard]] any_iterator operator+(difference_type n) const
			{
				assert_vtable<"iterator iterator::operator+(difference_type) const">(m_vtable->iter_add);
				return {m_vtable, m_vtable->iter_add(m_value, n)};
			}
			/** Returns an iterator located \a n elements before the underlying iterator.
			 * @throw bad_facet_function If the underlying iterator type is not a random-access iterator. */
			[[nodiscard]] any_iterator operator-(difference_type n) const
			{
				assert_vtable<"iterator iterator::operator-(difference_type) const">(m_vtable->iter_sub);
				return {m_vtable, m_vtable->iter_sub(m_value, n)};
			}

			/** Returns difference between underlying iterators of `this` and \a other.
			 * @throw bad_facet_function If the underlying iterator type is not a random-access iterator. */
			[[nodiscard]] difference_type operator-(const any_iterator &other) const
			{
				assert_vtable<"difference_type iterator::operator-(const iterator &) const">(m_vtable->iter_diff);
				return m_vtable->iter_diff(m_value, other.m_value);
			}

			/** Dereferences the underlying iterator. */
			[[nodiscard]] any operator*() const
			{
				assert_vtable<"value_type iterator::operator*()">(m_vtable->iter_deref);
				return m_vtable->iter_deref(m_value);
			}

			[[nodiscard]] bool operator==(const any_iterator &other) const { return m_value == other.m_value; }
			[[nodiscard]] bool operator!=(const any_iterator &other) const { return m_value != other.m_value; }
			[[nodiscard]] bool operator>=(const any_iterator &other) const { return m_value >= other.m_value; }
			[[nodiscard]] bool operator<=(const any_iterator &other) const { return m_value <= other.m_value; }
			[[nodiscard]] bool operator>(const any_iterator &other) const { return m_value > other.m_value; }
			[[nodiscard]] bool operator<(const any_iterator &other) const { return m_value < other.m_value; }

		private:
			const iterator_vtable *m_vtable = nullptr;
			any m_value;
		};

		struct range_vtable
		{
			iterator_vtable iter_funcs;
			iterator_vtable const_iter_funcs;

			type_info (*value_type)() = nullptr;

			any (*begin)(any &) = nullptr;
			any (*cbegin)(const any &) = nullptr;
			any (*end)(any &) = nullptr;
			any (*cend)(const any &) = nullptr;

			any (*at)(any &, std::size_t) = nullptr;
			any (*at_const)(const any &, std::size_t) = nullptr;

			bool (*empty)(const any &) = nullptr;
			std::size_t (*size)(const any &) = nullptr;
		};
	}

	/** Facet type implementing a generic range. */
	class range : public facet<detail::range_vtable>
	{
		using base_t = facet<detail::range_vtable>;

	public:
		using iterator = detail::any_iterator;
		using const_iterator = detail::any_iterator;
		using difference_type = std::ptrdiff_t;
		using size_type = std::size_t;

	public:
		using base_t::facet;
		using base_t::operator=;

		/** Returns type info of the value type of the range. */
		[[nodiscard]] type_info value_type() const noexcept { return vtable()->value_type(); }

		/** Returns a type-erased begin iterator of the underlying range. If the underlying
		 * range is const-qualified, returns a constant iterator instead. */
		[[nodiscard]] iterator begin()
		{
			if (!instance().is_const())
				return {&vtable()->iter_funcs, vtable()->begin(instance())};
			else
				return cbegin();
		}
		/** Returns a type-erased constant begin iterator of the underlying range. */
		[[nodiscard]] const_iterator cbegin() const { return {&vtable()->const_iter_funcs, vtable()->cbegin(instance())}; }
		/** @copydoc cbegin */
		[[nodiscard]] const_iterator begin() const { return cbegin(); }

		/** Returns a type-erased end iterator of the underlying range. If the underlying
		 * range is const-qualified, returns a constant iterator instead. */
		[[nodiscard]] iterator end()
		{
			if (!instance().is_const())
				return {&vtable()->iter_funcs, vtable()->end(instance())};
			else
				return cend();
		}
		/** Returns a type-erased constant end iterator of the underlying range. */
		[[nodiscard]] const_iterator cend() const { return {&vtable()->const_iter_funcs, vtable()->cend(instance())}; }
		/** @copydoc cend */
		[[nodiscard]] const_iterator end() const { return cend(); }

		/** Checks if the underlying range is empty. */
		[[nodiscard]] bool empty() const { return vtable()->empty(instance()); }
		/** Returns size of the underlying range.
		 * @throw bad_facet_function If the underlying range type is not a sized range. */
		[[nodiscard]] size_type size() const { return base_t::checked_invoke<&vtable_type::size, "size_type size() const">(instance()); }

		/** Returns element located at index \a n within the underlying range.
		 * @throw bad_facet_function If the underlying range type is not a random-access range. */
		[[nodiscard]] any at(size_type n) { return base_t::checked_invoke<&vtable_type::at, "value_type at(size_type)">(instance(), n); }
		/** @copydoc at */
		[[nodiscard]] any at(size_type n) const { return base_t::checked_invoke<&vtable_type::at_const, "value_type at(size_type) const">(instance(), n); }
	};

	template<std::ranges::input_range T>
	struct impl_facet<range, T>
	{
	private:
		using difference_type = std::ranges::range_difference_t<T>;
		using iterator = std::decay_t<decltype(std::ranges::begin(std::declval<T &>()))>;
		using const_iterator = std::decay_t<decltype(std::ranges::begin(std::declval<const T &>()))>;

		template<typename Iter>
		[[nodiscard]] constexpr static detail::iterator_vtable make_iterator_vtable()
		{
			detail::iterator_vtable result = {};
			result.iter_deref = +[](const any &iter)
			{
				auto &iter_ref = *iter.try_as<Iter>();
				return forward_any(*iter_ref);
			};
			if constexpr (std::ranges::forward_range<T>)
			{
				result.iter_pre_inc = +[](any &iter) { ++(*iter.try_as<Iter>()); };
				result.iter_post_inc = +[](any &iter) { return forward_any((*iter.try_as<Iter>())++); };
			}
			if constexpr (std::ranges::bidirectional_range<T>)
			{
				result.iter_pre_dec = +[](any &iter) { --(*iter.try_as<Iter>()); };
				result.iter_post_dec = +[](any &iter) { return forward_any((*iter.try_as<Iter>())--); };
			}
			if constexpr (std::ranges::random_access_range<T>)
			{
				result.iter_eq_add = +[](any &iter, std::ptrdiff_t n)
				{
					const auto diff = static_cast<difference_type>(n);
					(*iter.try_as<Iter>()) += diff;
				};
				result.iter_eq_sub = +[](any &iter, std::ptrdiff_t n)
				{
					const auto diff = static_cast<difference_type>(n);
					(*iter.try_as<Iter>()) -= diff;
				};
				result.iter_add = +[](const any &iter, std::ptrdiff_t n)
				{
					const auto diff = static_cast<difference_type>(n);
					return forward_any(*iter.try_as<Iter>() + diff);
				};
				result.iter_sub = +[](const any &iter, std::ptrdiff_t n)
				{
					const auto diff = static_cast<difference_type>(n);
					return forward_any(*iter.try_as<Iter>() - diff);
				};
				result.iter_diff = +[](const any &a, const any &b)
				{
					return static_cast<std::ptrdiff_t>(*a.try_as<Iter>() - *b.try_as<Iter>());
				};
			}
			return result;
		}

		[[nodiscard]] constexpr static detail::range_vtable make_vtable()
		{
			detail::range_vtable result = {};

			result.iter_funcs = make_iterator_vtable<iterator>();
			result.const_iter_funcs = make_iterator_vtable<const_iterator>();
			result.value_type = +[]() { return type_info::get<std::ranges::range_value_t<T>>(); };

			result.begin = +[](any &range)
			{
				auto &range_ref = range.as<T>();
				return make_any<iterator>(std::ranges::begin(range_ref));
			};
			result.cbegin = +[](const any &range)
			{
				auto &range_ref = range.as<T>();
				return make_any<const_iterator>(std::ranges::begin(range_ref));
			};
			result.end = +[](any &range)
			{
				auto &range_ref = range.as<T>();
				return make_any<iterator>(std::ranges::end(range_ref));
			};
			result.cend = +[](const any &range)
			{
				auto &range_ref = range.as<T>();
				return make_any<const_iterator>(std::ranges::end(range_ref));
			};

			result.empty = +[](const any &range)
			{
				auto &range_ref = range.as<T>();
				return static_cast<bool>(std::ranges::empty(range_ref));
			};

			if constexpr (std::ranges::sized_range<T>)
				result.size = +[](const any &range)
				{
					auto &range_ref = range.as<T>();
					return static_cast<std::size_t>(std::ranges::size(range_ref));
				};

			if constexpr (std::ranges::random_access_range<T>)
			{
				result.at = +[](any &range, std::size_t n)
				{
					auto &range_ref = range.as<T>();
					const auto diff = static_cast<difference_type>(n);
					return forward_any(std::ranges::begin(range_ref)[diff]);
				};
				result.at_const = +[](const any &range, std::size_t n)
				{
					auto &range_ref = range.as<T>();
					const auto diff = static_cast<difference_type>(n);
					return forward_any(std::ranges::begin(range_ref)[diff]);
				};
			}

			return result;
		}

	public:
		constexpr static detail::range_vtable value = make_vtable();
	};
}

/** Type initializer overload for types implementing `std::ranges::input_range`. */
template<std::ranges::input_range R>
struct reflex::type_init<R> { void operator()(reflex::type_factory<R> f) { f.template implement_facet<reflex::facets::range>(); } };