/*
 * Created by switchblade on 2023-03-13.
 */

#pragma once

#include <span>

#include "fwd.hpp"

namespace reflex::detail
{
	enum type_flags
	{
		/* Used by any, property_data & argument_data */
		IS_CONST = 0x1,

		/* Used by any */
		IS_VALUE = 0x2,
		IS_OWNED = 0x4,

		/* Used by type_info */
		IS_NULL = 0x10,
		IS_VOID = 0x20,
		IS_ENUM = 0x40,
		IS_CLASS = 0x800,
		IS_POINTER = 0x100,
		IS_SIGNED_INT = 0x200,
		IS_UNSIGNED_INT = 0x400,
		IS_ARITHMETIC = 0x800,
	};

	constexpr type_flags &operator&=(type_flags &a, std::underlying_type_t<type_flags> b) noexcept { return a = static_cast<type_flags>(a & b); }
	constexpr type_flags &operator|=(type_flags &a, std::underlying_type_t<type_flags> b) noexcept { return a = static_cast<type_flags>(a | b); }
	constexpr type_flags &operator^=(type_flags &a, std::underlying_type_t<type_flags> b) noexcept { return a = static_cast<type_flags>(a ^ b); }

	class type_dtor
	{
		template<typename>
		struct instance_t;
		template<>
		struct instance_t<void>
		{
			constexpr instance_t(void (*destroy)(instance_t<void> *)) noexcept : destroy(destroy) {}

			void (*destroy)(instance_t<void> *);
		};
		template<typename Dtor>
		struct instance_t : instance_t<void>
		{
			constexpr instance_t(Dtor &&value) : instance_t<void>(+[](instance_t<void> *ptr) { delete static_cast<instance_t *>(ptr); }), value(std::forward<Dtor>(value)) {}

			Dtor value;
		};

	public:
		template<typename T, typename PC, typename Deleting>
		[[nodiscard]] static constexpr type_dtor bind(PC &&in_place, Deleting &&deleting) noexcept
		{
			using in_place_t = std::remove_cvref_t<PC>;
			using deleting_t = std::remove_cvref_t<Deleting>;

			type_dtor result;
			if constexpr (std::is_empty_v<in_place_t>)
				result.m_destroy_in_place = +[](void *ptr) { PC{}(static_cast<T *>(ptr)); };
			else
			{
				result.m_destroy_in_place_udata = +[](void *ptr, void *del) { static_cast<instance_t<in_place_t> *>(del)->value(static_cast<T *>(ptr)); };
				result.m_in_place_instance = new instance_t{std::forward<in_place_t>(in_place)};
			}
			if constexpr (std::is_empty_v<deleting_t>)
				result.m_destroy_deleting = +[](void *ptr) { deleting_t{}(static_cast<T *>(ptr)); };
			else
			{
				result.m_destroy_deleting_udata = +[](void *ptr, void *del) { static_cast<instance_t<deleting_t> *>(del)->value(static_cast<T *>(ptr)); };
				result.m_deleting_instance = new instance_t{std::forward<deleting_t>(deleting)};
			}
			return result;
		}
		template<typename T>
		[[nodiscard]] static constexpr type_dtor bind() noexcept { return bind<T>([](auto *ptr) { std::destroy_at(ptr); }, [](auto *ptr) { delete ptr; }); }

		constexpr ~type_dtor()
		{
			if (m_in_place_instance != nullptr) m_in_place_instance->destroy(m_in_place_instance);
			if (m_deleting_instance != nullptr) m_deleting_instance->destroy(m_deleting_instance);
		}

		constexpr void destroy_in_place(void *ptr) const
		{
			if (m_in_place_instance != nullptr)
				m_destroy_in_place_udata(ptr, m_in_place_instance);
			else
				m_destroy_in_place(ptr);
		}
		constexpr void destroy_deleting(void *ptr) const
		{
			if (m_in_place_instance != nullptr)
				m_destroy_deleting_udata(ptr, m_in_place_instance);
			else
				m_destroy_deleting(ptr);
		}

	private:
		union
		{
			void (*m_destroy_in_place)(void *) = nullptr;
			void (*m_destroy_in_place_udata)(void *, void *);
		};
		instance_t<void> *m_in_place_instance = nullptr;

		union
		{
			void (*m_destroy_deleting)(void *) = nullptr;
			void (*m_destroy_deleting_udata)(void *, void *);
		};
		instance_t<void> *m_deleting_instance = nullptr;
	};

	using type_handle = type_data *(*)(type_domain &) noexcept;

	struct argument_data
	{
		template<typename T>
		static const argument_data value;

		template<typename... Ts>
		[[nodiscard]] static constexpr std::span<const argument_data *const> args_list() noexcept
		{
			if constexpr (sizeof...(Ts) != 0)
				return auto_constant<std::array{&argument_data::value<Ts>...}>::value;
			else
				return {};
		}

		[[nodiscard]] static constexpr bool verify(std::span<const argument_data *const> expected, std::span<any> actual) noexcept;
		[[nodiscard]] static constexpr bool verify(std::span<const argument_data *const> expected, std::span<const argument_data *const> actual) noexcept;

		/* Type name is stored separately to allow for comparison without acquiring a domain. */
		std::string_view type_name;

		type_handle type;
		type_flags flags;
	};

	template<>
	class REFLEX_PUBLIC type_ctor<>
	{
	public:
		constexpr type_ctor() noexcept = default;
#ifdef REFLEX_HEADER_ONLY
		virtual ~type_ctor() = default;
#else
		virtual ~type_ctor();
#endif

		constexpr type_ctor(std::span<const argument_data *const> args) noexcept : args(args) {}

		template<typename T, typename... Ts, typename PC, typename AC>
		[[nodiscard]] static constexpr type_ctor *bind(PC &&in_place, AC &&allocating)
		{
			using in_place_t = std::remove_cvref_t<PC>;
			using allocating_t = std::remove_cvref_t<AC>;
			return type_ctor<T, in_place_t, allocating_t, Ts...>::bind(std::forward<PC>(in_place), std::forward<AC>(allocating));
		}
		template<typename T, typename... Ts>
		[[nodiscard]] static constexpr type_ctor *bind()
		{
			return bind<T, Ts...>([](T *ptr, Ts ...as) { std::construct_at(ptr, as...); }, [](Ts ...as) { return new T(as...); });
		}

		virtual void construct_in_place(void *, std::span<any>) = 0;
		[[nodiscard]] virtual void *construct_allocating(std::span<any>) = 0;

		type_ctor *next = nullptr;
		std::span<const argument_data *const> args;
	};

	struct type_data
	{
		template<typename T>
		[[nodiscard]] inline static type_data *bind(type_domain &) noexcept;

		std::string_view name;
		type_flags flags = {};
		std::size_t size = 0;

		type_handle remove_pointer;
		type_handle remove_extent;
		std::size_t extent = 0;

		/* `any` instance factories. */
		void (any::*any_copy_init)(type_info, const void *, void *) = nullptr;
		void (any::*any_copy_assign)(type_info, const void *, void *) = nullptr;

		type_dtor dtor;

		type_ctor<> *default_ctor;
		type_ctor<> *copy_ctor;
		type_ctor<> *ctor_list;
	};

	template<typename T>
	constexpr argument_data argument_data::value = {
			type_name_v<T>, type_data::bind<std::remove_cvref_t<T>>,
			std::is_const_v<std::remove_reference_t<T>> ? IS_CONST : type_flags{}
	};
}