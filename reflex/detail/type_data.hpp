/*
 * Created by switchblade on 2023-03-13.
 */

#pragma once

#include <functional>
#include <array>
#include <list>

#include "any.hpp"
#include "type_info.hpp"

namespace reflex
{
	namespace detail
	{
		std::size_t str_hash::operator()(const type_info &value) const { return tpp::seahash_hash<type_info>{}(value); }

		std::size_t str_cmp::operator()(const type_info &a, const type_info &b) const { return a == b; }
		std::size_t str_cmp::operator()(const std::string &a, const type_info &b) const { return std::string_view{a} == b; }
		std::size_t str_cmp::operator()(const type_info &a, const std::string &b) const { return a == std::string_view{b}; }
		std::size_t str_cmp::operator()(const type_info &a, const std::string_view &b) const { return a == b; }
		std::size_t str_cmp::operator()(const std::string_view &a, const type_info &b) const { return a == b; }

		using facet_table = tpp::dense_map<std::string_view, const void *, str_hash, str_cmp>;
		using enum_table = tpp::stable_map<std::string, any, str_hash, str_cmp>;

		struct type_base
		{
			type_handle type;
			base_cast cast_func;
		};

		using base_table = tpp::dense_map<std::string_view, type_base, str_hash, str_cmp>;

		template<typename T, typename B>
		[[nodiscard]] inline static type_base make_type_base() noexcept
		{
			constexpr auto cast = [](const void *ptr) noexcept -> const void *
			{
				auto *base = static_cast<std::add_const_t<B> *>(static_cast<std::add_const_t<T> *>(ptr));
				return static_cast<const void *>(base);
			};
			return {make_type_data<B>, cast};
		}

		struct type_conv
		{
			type_conv() noexcept = default;
			template<typename From, typename To, typename F>
			type_conv(std::in_place_type_t<From>, std::in_place_type_t<To>, F &&conv)
			{
				conv_func = [f = std::forward<F>(conv)](const void *data)
				{
					return make_any<To>(std::invoke(f, *static_cast<const From *>(data)));
				};
			}

			std::function<any(const void *)> conv_func;
		};

		using conv_table = tpp::dense_map<std::string_view, type_conv, str_hash, str_cmp>;

		template<typename From, typename To, typename F>
		[[nodiscard]] inline static type_conv make_type_conv(F &&conv)
		{
			return {std::in_place_type<From>, std::in_place_type<To>, std::forward<F>(conv)};
		}
		template<typename From, typename To>
		[[nodiscard]] inline static type_conv make_type_conv() noexcept
		{
			return make_type_conv<From, To>([](auto &value) { return static_cast<To>(value); });
		}

		struct arg_data
		{
			/* Return false when types are not compatible, or mutable ref is required, but got a constant. */
			[[nodiscard]] static bool is_invocable(const arg_data &a, const any &b) noexcept { return (!b.is_const() || (a.flags & (IS_CONST | IS_VALUE))) && b.type().compatible_with(a.type); }
			/* Return false when types don't exactly match, or mutable ref is required, but got a constant. */
			[[nodiscard]] static bool is_exact_invocable(const arg_data &a, const any &b) noexcept { return (!b.is_const() || (a.flags & (IS_CONST | IS_VALUE))) && b.type().name() == a.type; }

			[[nodiscard]] constexpr bool operator==(const arg_data &other) const noexcept { return type == other.type && flags == other.flags; }

			std::string_view type;
			type_flags flags;
		};

		template<typename T>
		[[nodiscard]] constexpr static arg_data make_arg_data() noexcept
		{
			type_flags flags = {};
			if constexpr (std::is_const_v<std::remove_reference_t<T>>) flags |= IS_CONST;
			if constexpr (!std::is_reference_v<T>) flags |= IS_VALUE;
			return {type_name_v<std::decay_t<T>>, flags};
		}

		template<typename... Ts>
		struct arg_list { constexpr static auto value = std::array{make_arg_data<Ts>()...}; };
		template<>
		struct arg_list<> { constexpr static std::span<const arg_data> value = {}; };

		struct type_ctor
		{
			template<typename T, typename... Ts, typename F, std::size_t... Is>
			[[nodiscard]] inline static any construct(std::index_sequence<Is...>, F &&f, std::span<any> args)
			{
				return std::invoke(f, (std::forward<Ts>(*args[Is].cast<Ts>().template get<Ts>()))...);
			}
			template<typename T, typename... Ts, typename F>
			[[nodiscard]] inline static any construct(F &&f, std::span<any> args)
			{
				return construct<T, Ts...>(std::make_index_sequence<sizeof...(Ts)>{}, std::forward<F>(f), args);
			}

			type_ctor() noexcept = default;
			template<typename T, typename... Ts, typename F>
			type_ctor(std::in_place_type_t<T>, type_pack_t<Ts...>, F &&func) : args(arg_list<Ts...>::value)
			{
				this->func = [f = std::forward<F>(func)](std::span<any> any_args) -> any
				{
					return construct<T, std::remove_reference_t<Ts>...>(f, any_args);
				};
			}

			std::span<const arg_data> args;
			std::function<any(std::span<any>)> func;
		};

		template<typename T, typename... Ts, typename F>
		[[nodiscard]] inline static type_ctor make_type_ctor(F &&func)
		{
			return {std::in_place_type<T>, type_pack<Ts...>, std::forward<F>(func)};
		}
		template<typename T, typename... Ts>
		[[nodiscard]] inline static type_ctor make_type_ctor() noexcept
		{
			return make_type_ctor<T, Ts...>([](Ts ...as) -> any { return make_any<T>(as...); });
		}

		struct type_cmp
		{
			bool (*cmp_eq)(const void *, const void *) = nullptr;
			bool (*cmp_ne)(const void *, const void *) = nullptr;
			bool (*cmp_ge)(const void *, const void *) = nullptr;
			bool (*cmp_le)(const void *, const void *) = nullptr;
			bool (*cmp_gt)(const void *, const void *) = nullptr;
			bool (*cmp_lt)(const void *, const void *) = nullptr;
		};

		using cmp_table = tpp::dense_map<std::string_view, type_cmp, str_hash, str_cmp>;

		template<typename T, typename U = T>
		[[nodiscard]] inline static type_cmp make_type_cmp() noexcept
		{
			type_cmp result;
			if constexpr (requires(const T &a, const U &b){ a == b; })
				result.cmp_eq = +[](const void *a, const void *b) { return *static_cast<const T *>(a) == *static_cast<const U *>(b); };
			if constexpr (requires(const T &a, const U &b){ a != b; })
				result.cmp_ne = +[](const void *a, const void *b) { return *static_cast<const T *>(a) != *static_cast<const U *>(b); };
			if constexpr (requires(const T &a, const U &b){ a >= b; })
				result.cmp_ge = +[](const void *a, const void *b) { return *static_cast<const T *>(a) >= *static_cast<const U *>(b); };
			if constexpr (requires(const T &a, const U &b){ a <= b; })
				result.cmp_le = +[](const void *a, const void *b) { return *static_cast<const T *>(a) <= *static_cast<const U *>(b); };
			if constexpr (requires(const T &a, const U &b){ a > b; })
				result.cmp_gt = +[](const void *a, const void *b) { return *static_cast<const T *>(a) > *static_cast<const U *>(b); };
			if constexpr (requires(const T &a, const U &b){ a < b; })
				result.cmp_lt = +[](const void *a, const void *b) { return *static_cast<const T *>(a) < *static_cast<const U *>(b); };
			return result;
		}

		struct any_funcs_t
		{
			void (any::*copy_init)(type_info, const void *, void *) = nullptr;
			void (any::*copy_assign)(type_info, const void *, void *) = nullptr;
		};

		template<typename T>
		[[nodiscard]] inline static any_funcs_t make_any_funcs() noexcept
		{
			any_funcs_t result;
			result.copy_init = &any::copy_from<T>;
			result.copy_assign = &any::assign_from<T>;
			return result;
		}

		struct type_data
		{
			[[nodiscard]] const void *find_facet(std::string_view type) const
			{
				if (auto iter = facet_list.find(type); iter != facet_list.end())
					return iter->second;
				else
					return nullptr;
			}
			[[nodiscard]] const type_base *find_base(std::string_view type) const
			{
				if (auto iter = base_list.find(type); iter != base_list.end())
					return &iter->second;
				else
					return nullptr;
			}
			[[nodiscard]] const type_conv *find_conv(std::string_view type) const
			{
				if (auto iter = conv_list.find(type); iter != conv_list.end())
					return &iter->second;
				else
					return nullptr;
			}
			[[nodiscard]] const type_cmp *find_cmp(std::string_view type) const
			{
				if (auto iter = cmp_list.find(type); iter != cmp_list.end())
					return &iter->second;
				else
					return nullptr;
			}

			[[nodiscard]] const type_ctor *find_ctor(std::span<any> args) const
			{
				const auto exact_pred = [&](const type_ctor &ctor)
				{
					if (ctor.args.size() != args.size()) return false;
					for (std::size_t i = 0; i < ctor.args.size() && i < args.size(); ++i)
					{
						if (!arg_data::is_exact_invocable(ctor.args[i], args[i]))
							return false;
					}
					return true;
				};
				const auto compat_pred = [&](const type_ctor &ctor)
				{
					if (ctor.args.size() != args.size()) return false;
					for (std::size_t i = 0; i < ctor.args.size() && i < args.size(); ++i)
					{
						if (!arg_data::is_invocable(ctor.args[i], args[i]))
							return false;
					}
					return true;
				};

				/* First search for exact matches, then search for matches with conversion. */
				for (const auto &ctor: ctor_list) if (exact_pred(ctor)) return &ctor;
				for (const auto &ctor: ctor_list) if (compat_pred(ctor)) return &ctor;
				return nullptr;
			}

			[[nodiscard]] type_ctor *find_ctor(std::span<const arg_data> args)
			{
				for (auto &ctor: ctor_list) if (std::ranges::equal(ctor.args, args)) return &ctor;
				return nullptr;
			}
			[[nodiscard]] const type_ctor *find_ctor(std::span<const arg_data> args) const
			{
				for (auto &ctor: ctor_list) if (std::ranges::equal(ctor.args, args)) return &ctor;
				return nullptr;
			}

			type_data() = default;
			explicit type_data(std::string_view name) : name(name) {}

			std::string_view name;
			type_flags flags = {};

			std::size_t size = 0;
			std::size_t alignment = 0;

			type_handle remove_pointer = nullptr;
			type_handle remove_extent = nullptr;
			std::size_t extent = 0;

			/* Facet vtables. */
			facet_table facet_list;
			/* Enumeration constants. */
			enum_table enum_list;
			/* Base types. */
			base_table base_list;
			/* Type conversions. */
			conv_table conv_list;

			/* Constructors & destructors. */
			std::function<void(void *)> dtor;
			std::list<type_ctor> ctor_list;

			/* Comparison functions. */
			cmp_table cmp_list;

			/* `any` functions. */
			any_funcs_t any_funcs;
		};

		static_assert(alignof(detail::type_data) > static_cast<std::size_t>(detail::ANY_FAGS_MAX));
	}

	constexpr std::string_view type_info::name() const noexcept { return valid() ? m_data->name : std::string_view{}; }
	constexpr std::size_t type_info::extent() const noexcept { return valid() ? m_data->extent : 0; }
	constexpr std::size_t type_info::size() const noexcept { return valid() ? m_data->size : 0; }
	constexpr std::size_t type_info::alignment() const noexcept { return valid() ? m_data->alignment : 0; }

	constexpr bool type_info::is_empty() const noexcept { return size() == 0; }
	constexpr bool type_info::is_void() const noexcept { return valid() && (m_data->flags & detail::IS_VOID); }
	constexpr bool type_info::is_nullptr() const noexcept { return valid() && (m_data->flags & detail::IS_NULL); }

	constexpr bool type_info::is_array() const noexcept { return extent() > 0; }
	constexpr bool type_info::is_enum() const noexcept { return valid() && (m_data->flags & detail::IS_ENUM); }
	constexpr bool type_info::is_class() const noexcept { return valid() && (m_data->flags & detail::IS_CLASS); }
	constexpr bool type_info::is_abstract() const noexcept { return valid() && (m_data->flags & detail::IS_ABSTRACT); }

	constexpr bool type_info::is_pointer() const noexcept { return valid() && (m_data->flags & detail::IS_POINTER); }
	constexpr bool type_info::is_integral() const noexcept { return valid() && (m_data->flags & (detail::IS_SIGNED_INT | detail::IS_UNSIGNED_INT)); }
	constexpr bool type_info::is_signed_integral() const noexcept { return valid() && (m_data->flags & detail::IS_SIGNED_INT); }
	constexpr bool type_info::is_unsigned_integral() const noexcept { return valid() && (m_data->flags & detail::IS_UNSIGNED_INT); }
	constexpr bool type_info::is_arithmetic() const noexcept { return valid() && (m_data->flags & detail::IS_ARITHMETIC); }

	type_info type_info::remove_extent() const noexcept { return valid() && m_data->remove_extent ? type_info{m_data->remove_extent, *m_db} : type_info{}; }
	type_info type_info::remove_pointer() const noexcept { return valid() && m_data->remove_pointer ? type_info{m_data->remove_pointer, *m_db} : type_info{}; }

	template<typename... Args>
	bool type_info::constructible_from() const { return constructible_from(detail::arg_list<Args...>::value); }

	template<typename T>
	void any::copy_init(type_info type, T *ptr)
	{
		if constexpr (std::is_const_v<T>)
			(this->*(type_data()->any_funcs.copy_init))(type, ptr, nullptr);
		else
			(this->*(type_data()->any_funcs.copy_init))(type, nullptr, ptr);
	}
	template<typename T>
	void any::copy_assign(type_info type, T *ptr)
	{
		if constexpr (std::is_const_v<T>)
			(this->*(type_data()->any_funcs.copy_assign))(type, ptr, nullptr);
		else
			(this->*(type_data()->any_funcs.copy_assign))(type, nullptr, ptr);
	}
}