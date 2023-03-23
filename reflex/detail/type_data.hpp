/*
 * Created by switchblade on 2023-03-13.
 */

#pragma once

#include <functional>
#include <array>
#include <list>

#include "any.hpp"

namespace reflex
{
	namespace detail
	{
		struct type_base
		{
			type_handle type;
			base_cast cast_func;
		};

		template<typename T, typename B>
		[[nodiscard]] inline static type_base make_type_base() noexcept
		{
			constexpr auto cast = [](const void *ptr) -> const void *
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
			[[nodiscard]] constexpr static bool is_invocable(const arg_data &a, const any &b) noexcept { return !(a.flags == IS_CONST && !b.is_const()) && b.type().compatible_with(a.type); }
			/* Return false when types don't exactly match, or mutable ref is required, but got a constant. */
			[[nodiscard]] constexpr static bool is_exact_invocable(const arg_data &a, const any &b) noexcept { return !(a.flags == IS_CONST && !b.is_const()) && b.type().name() != a.type; }

			std::string_view type;
			type_flags flags;
		};

		template<typename T>
		[[nodiscard]] constexpr static arg_data make_arg_data() noexcept
		{
			type_flags flags = {};
			if constexpr (std::is_const_v<T>) flags |= IS_CONST;
			if constexpr (!std::is_reference_v<T>) flags |= IS_VALUE;
			return {type_name_v<T>, flags};
		}

		template<typename... Ts>
		struct arg_list { constexpr static auto value = std::array{make_arg_data<Ts>()...}; };
		template<>
		struct arg_list<> { constexpr static std::span<const arg_data> value = {}; };

		struct type_ctor
		{
			type_ctor() noexcept = default;
			template<typename T, typename... Ts, typename AF, typename PF>
			type_ctor(std::in_place_type_t<T>, type_pack_t<Ts...>, AF &&allocating, PF &&placement) : args(arg_list<Ts...>::value)
			{
				allocating_func = [f = std::forward<AF>(allocating)](std::span<any> args) -> void *
				{
					return construct<T, std::remove_reference_t<Ts>...>(f, args);
				};
				placement_func = [f = std::forward<PF>(placement)](void *ptr, std::span<any> args)
				{
					construct_at<T, std::remove_reference_t<Ts>...>(f, void_cast<T>(ptr), args);
				};
			}

			std::span<const arg_data> args;
			std::function<void *(std::span<any>)> allocating_func;
			std::function<void(void *, std::span<any>)> placement_func;
		};

		template<typename T, typename... Ts, typename F, std::size_t... Is>
		[[nodiscard]] inline static T *construct(std::index_sequence<Is...>, F &&f, std::span<any> args)
		{
			return std::invoke(f, (std::forward<Ts>(*args[Is].cast<Ts>().template get<Ts>()))...);
		}
		template<typename T, typename... Ts, typename F>
		[[nodiscard]] inline static T *construct(F &&f, std::span<any> args)
		{
			return construct<T, Ts...>(std::make_index_sequence<sizeof...(Ts)>{}, std::forward<F>(f), args);
		}

		template<typename T, typename... Ts, typename F, std::size_t... Is>
		inline static void construct_at(std::index_sequence<Is...>, F &&f, T *ptr, std::span<any> args)
		{
			std::invoke(f, ptr, (std::forward<Ts>(*args[Is].cast<Ts>().template get<Ts>()))...);
		}
		template<typename T, typename... Ts, typename F>
		inline static void construct_at(F &&f, T *ptr, std::span<any> args)
		{
			construct_at<T, Ts...>(std::make_index_sequence<sizeof...(Ts)>{}, std::forward<F>(f), ptr, args);
		}

		template<typename T, typename... Ts, typename AF, typename PF>
		[[nodiscard]] inline static type_ctor make_type_ctor(AF &&allocating, PF &&placement)
		{
			return {std::in_place_type<T>, type_pack<Ts...>, std::forward<AF>(allocating), std::forward<PF>(placement)};
		}
		template<typename T, typename... Ts>
		[[nodiscard]] inline static type_ctor make_type_ctor() noexcept
		{
			constexpr auto placement = [](T *ptr, Ts ...as) { std::construct_at(ptr, as...); };
			constexpr auto allocating = [](Ts ...as) -> T * { return new T(as...); };
			return make_type_ctor<T, Ts...>(allocating, placement);
		}

		struct type_dtor
		{
			type_dtor() noexcept = default;
			template<typename T, typename DF, typename PF>
			type_dtor(std::in_place_type_t<T>, DF &&deleting, PF &&placement)
			{
				deleting_func = [f = std::forward<DF>(deleting)](void *ptr) { std::invoke(f, void_cast<T>(ptr)); };
				placement_func = [f = std::forward<PF>(placement)](void *ptr) { std::invoke(f, void_cast<T>(ptr)); };
			}

			std::function<void(void *)> deleting_func;
			std::function<void(void *)> placement_func;
		};

		template<typename T, typename DF, typename PF>
		[[nodiscard]] inline static type_dtor make_type_dtor(DF &&deleting, PF &&placement)
		{
			return {std::in_place_type<T>, std::forward<DF>(deleting), std::forward<PF>(placement)};
		}
		template<typename T>
		[[nodiscard]] inline static type_dtor make_type_dtor() noexcept
		{
			return make_type_dtor<T>([](T *ptr) { delete ptr; }, [](T *ptr) { std::destroy_at(ptr); });
		}

		/* Properties can either be bound at compile-time, or created dynamically from an object instance.
		 * When created dynamically, a generator function is used to fill the dynamic property table, the generator
		 * is bound from a user-provided functor that accepts an instance pointer of type `T` or `const T` and
		 * a property factory functor with signature `void(string-like, getter, setter)`.
		 *
		 * Generator would then invoke the property factory for all dynamically-created properties, which would then be
		 * inserted into the dynamic property table. When property list is requested on an object of a reflected type,
		 * the property list is obtained from both static and dynamic properties of the type, as well as its parents. */
		struct type_prop
		{
			type_prop() noexcept = default;
			template<typename T, typename GF, typename SF>
			type_prop(std::in_place_type_t<T>, GF &&getter, SF &&setter)
			{
				getter_func = [f = std::forward<GF>(getter)](const void *cdata, void *data) -> any
				{
					if (cdata != nullptr)
						return forward_any(std::invoke(f, static_cast<std::add_const_t<T> *>(cdata)));
					else
						return forward_any(std::invoke(f, static_cast<T *>(data)));
				};
				setter_func = [f = std::forward<SF>(setter)](void *obj, any value)
				{
					std::invoke(f, void_cast<T>(obj), std::move(value));
				};
			}

			std::function<any(const void *, void *)> getter_func;
			std::function<void(void *, any)> setter_func;
		};
		struct prop_hash
		{
			using is_transparent = std::true_type;

			[[nodiscard]] std::size_t operator()(const std::string &value) const
			{
				return tpp::seahash_hash<std::string>{}(value);
			}
			[[nodiscard]] std::size_t operator()(const std::string_view &value) const
			{
				return tpp::seahash_hash<std::string_view>{}(value);
			}
		};
		struct prop_cmp
		{
			using is_transparent = std::true_type;

			[[nodiscard]] std::size_t operator()(const std::string &a, const std::string &b) const { return a == b; }
			[[nodiscard]] std::size_t operator()(const std::string &a, const std::string_view &b) const { return std::string_view{a} == b; }
			[[nodiscard]] std::size_t operator()(const std::string_view &a, const std::string &b) const { return a == std::string_view{b}; }
		};

		using prop_table = tpp::stable_map<std::string, type_prop, prop_hash, prop_cmp>;
		using prop_gen_t = std::function<void(prop_table &, const void *, void *)>;

		template<typename T, typename GF, typename SF>
		[[nodiscard]] inline static type_prop make_type_prop(GF &&getter, SF &&setter)
		{
			return {std::in_place_type<T>, std::forward<GF>(getter), std::forward<SF>(setter)};
		}
		template<typename T, typename F>
		[[nodiscard]] inline static prop_gen_t make_prop_gen(F &&generator)
		{
			return [f = std::forward<F>(generator)](prop_table &table, const void *cdata, void *data)
			{
				auto inserter = [&table]<typename GF, typename SF>(std::string_view name, GF &&getter, SF &&setter)
				{
					table.emplace_or_replace(name, std::in_place_type<T>,
					                         std::forward<GF>(getter),
					                         std::forward<SF>(setter));
				};
				if (cdata != nullptr)
					std::invoke(f, static_cast<std::add_const_t<T> *>(cdata), inserter);
				else
					std::invoke(f, static_cast<T *>(data), inserter);
			};
		}
		template<typename T, auto T::*Member>
		[[nodiscard]] inline static type_prop make_type_prop() noexcept
		{
			using mem_t = std::remove_cvref_t<decltype(std::declval<T>().*Member)>;
			constexpr auto getter = [](const void *cdata, void *data) -> any
			{
				if (cdata != nullptr)
					return forward_any(static_cast<std::add_const_t<T> *>(cdata)->*Member);
				else
					return forward_any(static_cast<T *>(data)->*Member);
			};
			constexpr auto setter = [](void *obj, any value)
			{
				(void_cast<T>(obj)->*Member) = *value.cast<mem_t>().template get<const mem_t>();
			};
			return {getter, setter};
		}

		struct type_data
		{
			[[nodiscard]] const type_base *find_base(std::string_view name) const
			{
				if (auto iter = base_list.find(name); iter != base_list.end())
					return &iter->second;
				else
					return nullptr;
			}
			[[nodiscard]] const type_conv *find_conv(std::string_view name) const
			{
				if (auto iter = conv_list.find(name); iter != conv_list.end())
					return &iter->second;
				else
					return nullptr;
			}
			[[nodiscard]] const type_prop *find_prop(std::string_view name) const
			{
				if (auto iter = static_props.find(name); iter != static_props.end())
					return &iter->second;
				else
					return nullptr;
			}

			std::string_view name;
			type_flags flags = {};
			std::size_t size = 0;

			type_handle remove_pointer;
			type_handle remove_extent;
			std::size_t extent = 0;

			/* Base types. */
			tpp::stable_map<std::string_view, type_base> base_list;
			/* Type conversions. */
			tpp::stable_map<std::string_view, type_conv> conv_list;
			/* Enumeration constants. */
			tpp::stable_map<std::string, any> enums;

			/* `any` copy-initialization factories. */
			void (any::*any_copy_init)(type_info, const void *, void *) = nullptr;
			void (any::*any_copy_assign)(type_info, const void *, void *) = nullptr;

			/* Constructors & destructors. */
			std::list<type_ctor> ctor_list;
			type_ctor *default_ctor = nullptr;
			type_ctor *copy_ctor = nullptr;
			type_dtor dtor;

			/* Instance properties (both statically-bound & dynamic). */
			prop_table static_props;
			prop_gen_t dyn_prop_gen;
		};
	}

	constexpr std::string_view type_info::name() const noexcept { return m_data->name; }
	constexpr std::size_t type_info::extent() const noexcept { return m_data->extent; }
	constexpr std::size_t type_info::size() const noexcept { return m_data->size; }

	constexpr bool type_info::is_empty() const noexcept { return size() == 0; }
	constexpr bool type_info::is_void() const noexcept { return m_data->flags & detail::IS_VOID; }
	constexpr bool type_info::is_nullptr() const noexcept { return m_data->flags & detail::IS_NULL; }

	constexpr bool type_info::is_array() const noexcept { return extent() > 0; }
	constexpr bool type_info::is_enum() const noexcept { return m_data->flags & detail::IS_ENUM; }
	constexpr bool type_info::is_class() const noexcept { return m_data->flags & detail::IS_CLASS; }

	constexpr bool type_info::is_pointer() const noexcept { return m_data->flags & detail::IS_POINTER; }
	constexpr bool type_info::is_integral() const noexcept { return m_data->flags & (detail::IS_SIGNED_INT | detail::IS_UNSIGNED_INT); }
	constexpr bool type_info::is_signed_integral() const noexcept { return m_data->flags & detail::IS_SIGNED_INT; }
	constexpr bool type_info::is_unsigned_integral() const noexcept { return m_data->flags & detail::IS_UNSIGNED_INT; }
	constexpr bool type_info::is_arithmetic() const noexcept { return m_data->flags & detail::IS_ARITHMETIC; }

	type_info type_info::remove_extent() const noexcept { return {m_data->remove_extent, *m_db}; }
	type_info type_info::remove_pointer() const noexcept { return {m_data->remove_pointer, *m_db}; }

	template<typename T>
	void any::copy_init(type_info type, T *ptr)
	{
		if constexpr (std::is_const_v<T>)
			(this->*(m_type->any_copy_init))(type, ptr, nullptr);
		else
			(this->*(m_type->any_copy_init))(type, nullptr, ptr);
	}
	template<typename T>
	void any::copy_assign(type_info type, T *ptr)
	{
		if constexpr (std::is_const_v<T>)
			(this->*(m_type->any_copy_assign))(type, ptr, nullptr);
		else
			(this->*(m_type->any_copy_assign))(type, nullptr, ptr);
	}
}