/*
 * Created by switchblade on 2023-03-25.
 */

#pragma once

#include "type_info.hpp"

namespace reflex
{
	template<typename T>
	[[nodiscard]] inline type_info type_of(T &&) requires std::derived_from<std::decay_t<T>, object>;

	/** @brief Base type used to attach reflection type info to an object instance.
	 *
	 * Children of this type must implement the `type_info do_type_of() const` protected virtual function.
	 * This function is used to obtain the actual instance type of the children of `object`. A convenience macro
	 * `REFLEX_DEFINE_OBJECT(T)` is provided and can be used to generate the required overloads. */
	class REFLEX_PUBLIC object
	{
		template<typename T>
		friend type_info type_of(T &&) requires std::derived_from<std::decay_t<T>, object>;

	public:
		virtual ~object() = 0;

	protected:
		/** Function used to obtain the instance type of `this`. */
		[[nodiscard]] virtual type_info do_type_of() const { return type_info::get<object>(); }
	};

	/** Returns the underlying type of \a value. */
	template<typename T>
	[[nodiscard]] type_info type_of(T &&obj) requires std::derived_from<std::decay_t<T>, object>
	{
		return static_cast<const object &>(obj).do_type_of();
	}

	namespace detail
	{
		[[nodiscard]] REFLEX_PUBLIC object *checked_object_cast(object *ptr, type_info from, type_info to) noexcept;
	}

	/** @brief Dynamically casts an object of type \a From to type \a To.
	 *
	 * If \a From is a child of \a To, returns `static_cast<To *>(ptr)`.
	 * Otherwise, if \a From an \a To are derived from `object`, and the underlying type of the
	 * object pointed to by \a ptr is a child of or same as \a To, equivalent to `static_cast<To *>(static_cast<[const] object *>(ptr))`.
	 * Otherwise, returns `nullptr`.
	 *
	 * @return Pointer \a ptr, dynamically cast to \a From, or `nullptr` if \a From cannot be cast to \a To. */
	template<typename To, typename From>
	[[nodiscard]] inline To *object_cast(From *ptr)
	{
		static_assert(std::same_as<take_const_t<To, From>, To>, "object_cast cannot cast away type qualifiers");
		if constexpr (std::same_as<To, From>)
			return ptr;
		else if constexpr (std::derived_from<From, To>)
			return static_cast<To *>(ptr);
		else if constexpr (std::derived_from<From, object> && std::derived_from<To, object>)
		{
			const auto from_type = type_of(*ptr);
			const auto to_type = type_info::get<To>();
			auto *obj = static_cast<take_const_t<object, From> *>(ptr);
			return static_cast<To *>(detail::checked_object_cast(const_cast<object *>(obj), from_type, to_type));
		}
		return nullptr;
	}

#ifdef REFLEX_HEADER_ONLY
	object::~object() = default;

	object *detail::checked_object_cast(object *ptr, type_info from, type_info to) noexcept
	{
		if (from == to || from.inherits_from(to))
			return ptr;
		else
			return nullptr;
	}
#endif
}

/** Convenience macro used to implement required `reflex::object` functions for type \a T. */
#define REFLEX_DEFINE_OBJECT(T) reflex::type_info do_type_of() const override { return reflex::type_info::get<T>(); }