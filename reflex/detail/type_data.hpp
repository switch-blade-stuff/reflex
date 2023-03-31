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

			constexpr arg_data() noexcept = default;
			constexpr arg_data(std::string_view type, type_flags flags) noexcept : type(type), flags(flags) {}

			[[nodiscard]] constexpr bool operator==(const arg_data &other) const noexcept { return type == other.type && flags == other.flags; }

			std::string_view type;
			type_flags flags = {};
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
				return std::invoke(f, (std::forward<Ts>(*args[Is].cast<Ts>().template try_get<Ts>()))...);
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

#ifdef _MSC_VER
		/* Allow signed/unsigned comparison. */
#pragma warning(push)
#pragma warning(disable : 4018)
#endif
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
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		struct any_funcs_t
		{
			void (any::*copy_init)(type_info, const void *, void *) = nullptr;
			void (any::*copy_assign)(type_info, const void *, void *) = nullptr;
		};

		template<typename T>
		[[nodiscard]] constexpr static any_funcs_t make_any_funcs() noexcept
		{
			any_funcs_t result;
			result.copy_init = &any::copy_from<T>;
			result.copy_assign = &any::assign_from<T>;
			return result;
		}

		/* Mutable type data initialized at runtime. */
		struct dynamic_type_data
		{
			[[nodiscard]] const void *find_facet(std::string_view type) const
			{
				if (auto iter = facets.find(type); iter != facets.end())
					return iter->second;
				else
					return nullptr;
			}
			[[nodiscard]] const type_base *find_base(std::string_view type) const
			{
				if (auto iter = bases.find(type); iter != bases.end())
					return &iter->second;
				else
					return nullptr;
			}

			[[nodiscard]] type_ctor *find_ctor(std::span<const arg_data> args)
			{
				for (auto &ctor: ctors) if (std::ranges::equal(ctor.args, args)) return &ctor;
				return nullptr;
			}
			[[nodiscard]] const type_ctor *find_ctor(std::span<const arg_data> args) const
			{
				for (auto &ctor: ctors) if (std::ranges::equal(ctor.args, args)) return &ctor;
				return nullptr;
			}
			[[nodiscard]] const type_ctor *find_ctor(std::span<const any> args) const
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
				for (const auto &ctor: ctors) if (exact_pred(ctor)) return &ctor;
				for (const auto &ctor: ctors) if (compat_pred(ctor)) return &ctor;
				return nullptr;
			}

			[[nodiscard]] const type_conv *find_conv(std::string_view type) const
			{
				if (auto iter = convs.find(type); iter != convs.end())
					return &iter->second;
				else
					return nullptr;
			}
			[[nodiscard]] const type_cmp *find_cmp(std::string_view type) const
			{
				if (auto iter = cmps.find(type); iter != cmps.end())
					return &iter->second;
				else
					return nullptr;
			}

			void clear()
			{
				facets.clear();
				enums.clear();
				bases.clear();
				ctors.clear();
				convs.clear();
				cmps.clear();
			}

			/* Enumeration constants. */
			enum_table enums;
			/* Base types. */
			base_table bases;
			/* Facet vtables. */
			facet_table facets;

			/* Type constructors. */
			std::list<type_ctor> ctors;
			/* Type conversions. */
			conv_table convs;
			/* Comparison functions. */
			cmp_table cmps;
		};
		/* Type data that can be constexpr-initialized. */
		struct constant_type_data
		{
			/* `type_data` initialization function. */
			void (type_data::*init_func)(database_impl &) = nullptr;
			/* `any` initialization functions. */
			any_funcs_t any_funcs;

			std::string_view name;
			type_flags flags = {};

			std::size_t size = 0;
			std::size_t alignment = 0;

			type_handle remove_pointer = nullptr;
			type_handle remove_extent = nullptr;
			std::size_t extent = 0;
		};

		/* Type data must be over-aligned since `any` uses bottom bits of its pointer for flags. */
		struct alignas(detail::ANY_FAGS_MAX + 1) type_data : constant_type_data, dynamic_type_data
		{
			type_data(const constant_type_data &cdata) : constant_type_data(cdata) {}

			template<typename T>
			REFLEX_COLD void impl_init(database_impl &db)
			{
				/* Constructors, destructors & conversions are only created for object types. */
				if constexpr (std::is_object_v<T>)
				{
					/* Add default & copy constructors. */
					if constexpr (std::is_default_constructible_v<T>)
						ctors.emplace_back(make_type_ctor<T>());
					if constexpr (std::is_copy_constructible_v<T>)
						ctors.emplace_back(make_type_ctor<T, std::add_const_t<T> &>());

					/* Add comparisons. */
					if constexpr (std::equality_comparable<T> || std::three_way_comparable<T>)
						cmps.emplace(type_name_v<T>, make_type_cmp<T>());

					/* Add enum underlying type overloads. */
					if constexpr (std::is_enum_v<T>)
					{
						ctors.emplace_back(make_type_ctor<T, std::underlying_type_t<T>>([](auto value) { return make_any<T>(static_cast<T>(value)); }));
						convs.emplace(type_name_v<std::underlying_type_t<T>>, make_type_conv<T, std::underlying_type_t<T>>());
						cmps.emplace(type_name_v<std::underlying_type_t<T>>, make_type_cmp<std::underlying_type_t<T>>());
					}
				}

				/* Invoke user type initializer. */
				if constexpr (requires(type_factory<T> f) { type_init<T>{}(f); })
				{
					type_factory<T> f{this, &db};
					type_init<T>{}(f);
				}
			}

			type_data &init(database_impl &db)
			{
				(this->*init_func)(db);
				return *this;
			}
			type_data &reset(database_impl &db)
			{
				/* Clear all dynamically-initialized metadata. */
				dynamic_type_data::clear();
				/* Re-initialize the metadata. */
				return init(db);
			}
		};

		template<typename T>
		[[nodiscard]] inline static const constant_type_data &make_constant_type_data() noexcept
		{
			constinit static const constant_type_data value =  []()
			{
				constant_type_data result;
				result.init_func = &type_data::impl_init<T>;
				result.any_funcs = make_any_funcs<T>();
				result.name = type_name_v<T>;

				if constexpr (std::same_as<T, void>)
					result.flags |= type_flags::IS_VOID;
				else
				{
					if constexpr (!std::is_empty_v<T>)
					{
						result.size = sizeof(T);
						result.alignment = alignof(T);
					}

					if constexpr (std::is_enum_v<T>) result.flags |= type_flags::IS_ENUM;
					if constexpr (std::is_class_v<T>) result.flags |= type_flags::IS_CLASS;
					if constexpr (std::is_pointer_v<T>) result.flags |= type_flags::IS_POINTER;
					if constexpr (std::is_abstract_v<T>) result.flags |= type_flags::IS_ABSTRACT;
					if constexpr (std::is_null_pointer_v<T>) result.flags |= type_flags::IS_NULL;

					if constexpr (std::is_arithmetic_v<T>) result.flags |= type_flags::IS_ARITHMETIC;
					if constexpr (std::signed_integral<T>) result.flags |= type_flags::IS_SIGNED_INT;
					if constexpr (std::unsigned_integral<T>) result.flags |= type_flags::IS_UNSIGNED_INT;

					result.remove_pointer = make_type_data<std::decay_t<std::remove_pointer_t<T>>>;
					result.remove_extent = make_type_data<std::decay_t<std::remove_extent_t<T>>>;
					result.extent = std::extent_v<T>;
				}
				return result;
			}();
			return value;
		}
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