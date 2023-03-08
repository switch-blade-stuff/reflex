/*
 * Created by switchblade on 2023-03-07.
 */

#pragma once

#include <string_view>
#include <cstring>

#include "detail/define.hpp"

namespace reflex
{
	/** @brief Fixed-size string for use with NTTP and constexpr. */
	template<typename C, std::size_t N, typename Traits = std::char_traits<C>>
	struct basic_const_string
	{
	public:
		typedef Traits traits_type;
		typedef C value_type;
		typedef value_type *pointer;
		typedef const value_type *const_pointer;
		typedef value_type &reference;
		typedef const value_type &const_reference;
		typedef pointer iterator;
		typedef const_pointer const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		constexpr static size_type npos = std::basic_string_view<C, Traits>::npos;

	private:
		[[nodiscard]] constexpr static size_type str_length(const C *str, std::size_t max) noexcept
		{
			if (!std::is_constant_evaluated())
			{
				if constexpr (std::same_as<C, char>)
					return static_cast<size_type>(strnlen(std::to_address(str), static_cast<size_type>(max)));
				else if constexpr (std::same_as<C, wchar_t>)
					return static_cast<size_type>(wcsnlen(std::to_address(str), static_cast<size_type>(max)));
				else if constexpr (std::same_as<C, char8_t>)
				{
					const auto ptr = reinterpret_cast<const char *>(std::to_address(str));
					return static_cast<size_type>(strnlen(ptr, static_cast<size_type>(max)));
				}
				else if constexpr (std::same_as<C, char16_t> && sizeof(char16_t) == sizeof(wchar_t))
				{
					const auto ptr = reinterpret_cast<const wchar_t *>(std::to_address(str));
					return static_cast<size_type>(wcsnlen(ptr, static_cast<size_type>(max)));
				}
				else if constexpr (std::same_as<C, char32_t> && sizeof(char32_t) == sizeof(wchar_t))
				{
					const auto ptr = reinterpret_cast<const wchar_t *>(std::to_address(str));
					return static_cast<size_type>(wcsnlen(ptr, static_cast<size_type>(max)));
				}
			}
			for (size_type i = 0;; ++i) if (i == max || str[i] == '\0') return i;
		}

	public:
		constexpr basic_const_string() = default;
		constexpr basic_const_string(const C (&str)[N]) : basic_const_string(str, N) {}
		constexpr basic_const_string(const C *str, size_type n) : basic_const_string(str, str + std::min(N, n)) {}

		template<std::size_t M>
		constexpr basic_const_string(const basic_const_string<C, M> &str) requires (M != N) : basic_const_string((str.data(), std::min(M, N))) {}

		template<std::forward_iterator I>
		constexpr basic_const_string(I first, I last) { std::copy(first, last, value); }
		template<std::ranges::forward_range R>
		constexpr basic_const_string(const R &range) : basic_const_string(std::ranges::begin(range), std::ranges::end(range)) {}

		/** Returns iterator to the start of the string. */
		[[nodiscard]] constexpr iterator begin() noexcept { return iterator{value}; }
		/** Returns iterator to the end of the string. */
		[[nodiscard]] constexpr iterator end() noexcept { return iterator{value + size()}; }
		/** Returns const iterator to the start of the string. */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{value}; }
		/** Returns const iterator to the end of the string. */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{value + size()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
		/** Returns reverse iterator to the end of the string. */
		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }
		/** Returns reverse iterator to the start of the string. */
		[[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }
		/** Returns const reverse iterator to the end of the string. */
		[[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator{end()}; }
		/** Returns const reverse iterator to the start of the string. */
		[[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator{begin()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** @copydoc rend */
		[[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

		/** Returns pointer to the string's data. */
		[[nodiscard]] constexpr pointer data() noexcept { return value; }
		/** @copydoc data */
		[[nodiscard]] constexpr const_pointer data() const noexcept { return value; }
		/** Returns reference to the element at the specified offset within the string. */
		[[nodiscard]] constexpr reference at(size_type i) noexcept { return value[i]; }
		/** @copydoc at */
		[[nodiscard]] constexpr reference operator[](size_type i) noexcept { return at(i); }
		/** Returns constant reference to the element at the specified offset within the string. */
		[[nodiscard]] constexpr const_reference at(size_type i) const noexcept { return value[i]; }
		/** @copydoc at */
		[[nodiscard]] constexpr const_reference operator[](size_type i) const noexcept { return at(i); }
		/** Returns reference to the element at the start of the string. */
		[[nodiscard]] constexpr reference front() noexcept { return value[0]; }
		/** Returns constant reference to the element at the start of the string. */
		[[nodiscard]] constexpr const_reference front() const noexcept { return value[0]; }
		/** Returns constant reference to the element at the end of the string. */
		[[nodiscard]] constexpr reference back() noexcept { return value[size() - 1]; }
		/** Returns constant reference to the element at the end of the string. */
		[[nodiscard]] constexpr const_reference back() const noexcept { return value[size() - 1]; }

		/** Returns size of the string (amount of value_type units). */
		[[nodiscard]] constexpr size_type size() const noexcept { return str_length(value, N); }
		/** @copydoc size */
		[[nodiscard]] constexpr size_type length() const noexcept { return size(); }
		/** Returns maximum value for size. */
		[[nodiscard]] constexpr size_type max_size() const noexcept { return npos; }
		/** Checks if the string is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

		/** Converts the static string to a string view. */
		[[nodiscard]] constexpr operator std::basic_string_view<C, Traits>() const noexcept
		{
			return std::basic_string_view<C, Traits>{data(), size()};
		}

		/** Finds left-most location of a substring within the string.
		 * @param c Substring to search for.
		 * @param pos Position to start the search at. */
		template<typename S>
		[[nodiscard]] constexpr size_type find(const S &str, size_type pos = 0) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			const std::basic_string_view<C, Traits> sv = str;
			return std::basic_string_view<C, Traits>{*this}.find(sv, pos);
		}
		/** @copydoc find */
		[[nodiscard]] constexpr size_type find(const value_type *str, size_type pos = 0) const noexcept
		{
			return find(std::basic_string_view<C, Traits>{str}, pos);
		}
		/** @copydoc find
		 * @param count Length of the substring. */
		[[nodiscard]] constexpr size_type find(const value_type *str, size_type pos, size_type count) const noexcept
		{
			return find(std::basic_string_view<C, Traits>{str, count}, pos);
		}
		/** Finds left-most location of a character within the string.
		 * @param c Character to search for.
		 * @param pos Position to start the search at. */
		[[nodiscard]] constexpr size_type find(value_type c, size_type pos = 0) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.find(c, pos);
		}

		/** Finds right-most location of a substring within the string. */
		template<typename S>
		[[nodiscard]] constexpr size_type rfind(const S &str, size_type pos = npos) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			const std::basic_string_view<C, Traits> sv = str;
			return std::basic_string_view<C, Traits>{*this}.rfind(sv, pos);
		}
		/** @copydoc rfind */
		[[nodiscard]] constexpr size_type rfind(const value_type *str, size_type pos = npos) const noexcept
		{
			return rfind(std::basic_string_view<C, Traits>{str}, pos);
		}
		/** @copydoc rfind
		 * @param count Length of the substring. */
		[[nodiscard]] constexpr size_type rfind(const value_type *str, size_type pos, size_type count) const noexcept
		{
			return rfind(std::basic_string_view<C, Traits>{str, count}, pos);
		}
		/** Finds left-most location of a character within the string.
		 * @param c Character to search for.
		 * @param pos Position to start the search at. */
		[[nodiscard]] constexpr size_type rfind(value_type c, size_type pos = npos) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.rfind(c, pos);
		}

		/** Finds left-most location of a character present within a substring.
		 * @param c Substring containing characters to search for.
		 * @param pos Position to start the search at. */
		template<typename S>
		[[nodiscard]] constexpr size_type find_first_of(const S &str, size_type pos = 0) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			const std::basic_string_view<C, Traits> sv = str;
			return std::basic_string_view<C, Traits>{*this}.find_first_of(sv, pos);
		}
		/** @copydoc find_first_of */
		[[nodiscard]] constexpr size_type find_first_of(const value_type *str, size_type pos = 0) const noexcept
		{
			return find_first_of(std::basic_string_view<C, Traits>{str}, pos);
		}
		/** @copydoc find_first_of
		 * @param count Length of the substring. */
		[[nodiscard]] constexpr size_type find_first_of(const value_type *str, size_type pos, size_type count) const noexcept
		{
			return find_first_of(std::basic_string_view<C, Traits>{str, count}, pos);
		}
		/** Finds left-most location of a character within the string (equivalent to `find(c, pos)`).
		 * @param c Character to search for.
		 * @param pos Position to start the search at. */
		[[nodiscard]] constexpr size_type find_first_of(value_type c, size_type pos = 0) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.find_first_of(c, pos);
		}

		/** Finds right-most location of a character present within a substring.
		 * @param c Substring containing characters to search for.
		 * @param pos Position to start the search at. */
		template<typename S>
		[[nodiscard]] constexpr size_type find_last_of(const S &str, size_type pos = npos) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			const std::basic_string_view<C, Traits> sv = str;
			return std::basic_string_view<C, Traits>{*this}.find_last_of(sv, pos);
		}
		/** @copydoc find_last_of */
		[[nodiscard]] constexpr size_type find_last_of(const value_type *str, size_type pos = npos) const noexcept
		{
			return find_last_of(std::basic_string_view<C, Traits>{str}, pos);
		}
		/** @copydoc find_last_of
		 * @param count Length of the substring. */
		[[nodiscard]] constexpr size_type find_last_of(const value_type *str, size_type pos, size_type count) const noexcept
		{
			return find_last_of(std::basic_string_view<C, Traits>{str, count}, pos);
		}
		/** Finds right-most location of a character within the string (equivalent to `rfind(c, pos)`).
		 * @param c Character to search for.
		 * @param pos Position to start the search at. */
		[[nodiscard]] constexpr size_type find_last_of(value_type c, size_type pos = npos) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.find_last_of(c, pos);
		}

		/** Finds left-most location of a character not present within a substring.
		 * @param c Substring containing characters to search for.
		 * @param pos Position to start the search at. */
		template<typename S>
		[[nodiscard]] constexpr size_type find_first_not_of(const S &str, size_type pos = 0) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			const std::basic_string_view<C, Traits> sv = str;
			return std::basic_string_view<C, Traits>{*this}.find_first_not_of(sv, pos);
		}
		/** @copydoc find_first_not_of */
		[[nodiscard]] constexpr size_type find_first_not_of(const value_type *str, size_type pos = 0) const noexcept
		{
			return find_first_not_of(std::basic_string_view<C, Traits>{str}, pos);
		}
		/** @copydoc find_first_not_of
		 * @param count Length of the substring. */
		[[nodiscard]] constexpr size_type find_first_not_of(const value_type *str, size_type pos, size_type count) const noexcept
		{
			return find_first_not_of(std::basic_string_view<C, Traits>{str, count}, pos);
		}
		/** Finds left-most location of a character not equal to `c`.
		 * @param c Character to search for.
		 * @param pos Position to start the search at. */
		[[nodiscard]] constexpr size_type find_first_not_of(value_type c, size_type pos = 0) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.find_first_not_of(c, pos);
		}

		/** Finds right-most location of a character not present within a substring.
		 * @param c Substring containing characters to search for.
		 * @param pos Position to start the search at. */
		template<typename S>
		[[nodiscard]] constexpr size_type find_last_not_of(const S &str, size_type pos = npos) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			const std::basic_string_view<C, Traits> sv = str;
			return std::basic_string_view<C, Traits>{*this}.find_last_not_of(sv, pos);
		}
		/** @copydoc find_last_not_of */
		[[nodiscard]] constexpr size_type find_last_not_of(const value_type *str, size_type pos = npos) const noexcept
		{
			return find_last_not_of(std::basic_string_view<C, Traits>{str}, pos);
		}
		/** @copydoc find_last_not_of
		 * @param count Length of the substring. */
		[[nodiscard]] constexpr size_type find_last_not_of(const value_type *str, size_type pos, size_type count) const noexcept
		{
			return find_last_not_of(std::basic_string_view<C, Traits>{str, count}, pos);
		}
		/** Finds right-most location of a character not equal to `c`.
		 * @param c Character to search for.
		 * @param pos Position to start the search at. */
		[[nodiscard]] constexpr size_type find_last_not_of(value_type c, size_type pos = npos) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.find_last_not_of(c, pos);
		}

		/** Checks if the string contains a substring.
		 * @param c Substring to search for. */
		template<typename S>
		[[nodiscard]] constexpr size_type contains(const S &str) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			return find(str) != npos;
		}
		/** @copydoc contains */
		[[nodiscard]] constexpr size_type contains(const value_type *str) const noexcept { return find(str) != npos; }
		/** Checks if the string contains a character.
		 * @param c Character to search for. */
		[[nodiscard]] constexpr size_type contains(value_type c) const noexcept { return find(c) != npos; }

		/** Checks if the string starts with a substring.
		 * @param c Substring to search for. */
		template<typename S>
		[[nodiscard]] constexpr size_type starts_with(const S &str) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			const std::basic_string_view<C, Traits> sv = str;
			return std::basic_string_view<C, Traits>{*this}.starts_with(sv);
		}
		/** @copydoc starts_with */
		[[nodiscard]] constexpr size_type starts_with(const value_type *str) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.starts_with(str);
		}
		/** Checks if the string starts with a character.
		 * @param c Character to search for. */
		[[nodiscard]] constexpr size_type starts_with(value_type c) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.starts_with(c);
		}

		/** Checks if the string ends with a substring.
		 * @param c Substring to search for. */
		template<typename S>
		[[nodiscard]] constexpr size_type ends_with(const S &str) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			const std::basic_string_view<C, Traits> sv = str;
			return std::basic_string_view<C, Traits>{*this}.ends_with(sv);
		}
		/** @copydoc starts_with */
		[[nodiscard]] constexpr size_type ends_with(const value_type *str) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.ends_with(str);
		}
		/** Checks if the string ends with a character.
		 * @param c Character to search for. */
		[[nodiscard]] constexpr size_type ends_with(value_type c) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.ends_with(c);
		}

		/** Compares the string with another.
		 * @param str String to compare with. */
		template<typename S>
		[[nodiscard]] constexpr int compare(const S &str) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			const std::basic_string_view<C, Traits> sv = str;
			return std::basic_string_view<C, Traits>{*this}.compare(sv);
		}
		/** @copydoc compare */
		[[nodiscard]] constexpr int compare(const value_type *str) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.compare(str);
		}
		/** @copydoc compare
		 * @param pos1 Position of the first character from this string to compare.
		 * @param count1 Amount of characters from this string to compare. */
		template<typename S>
		[[nodiscard]] constexpr int compare(size_type pos1, size_type count1, const S &str) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			const std::basic_string_view<C, Traits> sv = str;
			return std::basic_string_view<C, Traits>{*this}.compare(pos1, count1, sv);
		}
		/** @copydoc compare */
		[[nodiscard]] constexpr int compare(size_type pos1, size_type count1, const value_type *str) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.compare(pos1, count1, str);
		}
		/** @copydoc compare
		 * @param pos2 Position of the first character from the other string to compare.
		 * @param count2 Amount of characters from the other string to compare. */
		template<typename S>
		[[nodiscard]] constexpr int compare(size_type pos1, size_type count1, size_type pos2, size_type count2, const S &str) const noexcept requires std::is_convertible_v<const S, std::basic_string_view<C, Traits>>
		{
			const std::basic_string_view<C, Traits> sv = str;
			return std::basic_string_view<C, Traits>{*this}.compare(pos1, count1, pos2, count2, sv);
		}
		/** @copydoc compare */
		[[nodiscard]] constexpr int compare(size_type pos1, size_type count1, size_type pos2, size_type count2, const value_type *str) const noexcept
		{
			return std::basic_string_view<C, Traits>{*this}.compare(pos1, count1, pos2, count2, str);
		}

		[[nodiscard]] friend constexpr auto operator<=>(const basic_const_string &a, const basic_const_string &b) noexcept
		{
			return std::basic_string_view<C, Traits>{a} <=> std::basic_string_view<C, Traits>{b};
		}
		[[nodiscard]] friend constexpr bool operator==(const basic_const_string &a, const basic_const_string &b) noexcept
		{
			return std::basic_string_view<C, Traits>{a} == std::basic_string_view<C, Traits>{b};
		}

		C value[N] = {0};
	};

	template<typename C, std::size_t N>
	basic_const_string(const C (&)[N]) -> basic_const_string<C,  N, std::char_traits<C>>;

	template<typename To, typename ToT = std::char_traits<To>, typename From, typename FromT = std::char_traits<From>, std::size_t N>
	[[nodiscard]] constexpr basic_const_string<To, N, ToT> const_string_cast(basic_const_string<From, N, FromT> str) noexcept
	{
		basic_const_string<To, N, ToT> result;
		for (std::size_t i = 0; i < N; ++i) result[i] = static_cast<To>(str[i]);
		return result;
	}
	template<typename To, typename ToT = std::char_traits<To>, typename From, std::size_t N>
	[[nodiscard]] constexpr basic_const_string<To, N, ToT> const_string_cast(const From (&str)[N]) noexcept
	{
		return const_string_cast<To, ToT>(basic_const_string<From, N>{str});
	}

	template<typename C, typename T = std::char_traits<C>, std::size_t N, std::size_t M>
	[[nodiscard]] constexpr basic_const_string<C, N + M, T> operator+(const basic_const_string<C, N, T> &a, const basic_const_string<C, M, T> &b) noexcept
	{
		basic_const_string<C, N + M, T> result;
		std::copy(a.data(), a.data() + a.size(), result.data());
		std::copy(b.data(), b.data() + b.size(), result.data() + a.size());
		return result;
	}

	template<std::size_t I, typename C, std::size_t N, typename T>
	[[nodiscard]] constexpr auto &get(basic_const_string<C, N, T> &s) noexcept
	{
		static_assert(I < N);
		return s.value[I];
	}
	template<std::size_t I, typename C, std::size_t N, typename T>
	[[nodiscard]] constexpr auto &get(const basic_const_string<C, N, T> &s) noexcept
	{
		static_assert(I < N);
		return s.value[I];
	}

	template<std::size_t N>
	using const_string = basic_const_string<char, N>;
	template<std::size_t N>
	using static_wstring = basic_const_string<wchar_t, N>;
	template<std::size_t N>
	using static_u8string = basic_const_string<char8_t, N>;
	template<std::size_t N>
	using static_u16string = basic_const_string<char16_t, N>;
	template<std::size_t N>
	using static_u32string = basic_const_string<char32_t, N>;
}     // namespace sek

template<typename C, std::size_t N, typename T>
struct std::hash<reflex::basic_const_string<C, N, T>>
{
	[[nodiscard]] constexpr std::size_t operator()(const reflex::basic_const_string<C, N, T> &s) const noexcept
	{
		return std::hash<std::string_view>{}(static_cast<std::string_view>(s));
	}
};

template<typename C, std::size_t N, typename T>
struct std::tuple_size<reflex::basic_const_string<C, N, T>> : std::integral_constant<std::size_t, N> {};
template<std::size_t I, typename C, std::size_t N, typename T>
struct std::tuple_element<I, reflex::basic_const_string<C, N, T>> : std::type_identity<C> {};