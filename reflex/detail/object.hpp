/*
 * Created by switchblade on 2023-03-25.
 */

#pragma once

#include "database.hpp"

namespace reflex
{
	template<typename T>
	[[nodiscard]] inline type_info type_of(T &&) requires std::derived_from<std::decay_t<T>, object>;

	/** @brief Base type used to attach reflection type info to an object instance.
	 *
	 * Children of this type must implement the `type_info do_type_of() const` protected virtual function.
	 * This function is used to obtain the actual instance type of the children of `object`. A convenience macro
	 * `REFLEX_DEFINE_OBJECT(T)` is provided and can be used to generate the required overloads. */
	class REFLEX_VISIBLE object
	{
		template<typename T>
		friend type_info type_of(T &&) requires std::derived_from<std::decay_t<T>, object>;

	public:
		REFLEX_PUBLIC virtual ~object() = 0;

	protected:
		/** Function used to obtain the instance type of `this`. */
		[[nodiscard]] virtual type_info do_type_of() const { return type_info::get<object>(); }
	};

#ifdef REFLEX_HEADER_ONLY
	object::~object() = default;
#endif

	/** Returns the underlying type of \a value. */
	template<typename T>
	[[nodiscard]] type_info type_of(T &&obj) requires std::derived_from<std::decay_t<T>, object> { return static_cast<const object &>(obj).do_type_of(); }

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
		static_assert(std::same_as<take_const_t<To, From>, To>,
		              "Cannot cast away type qualifiers");
		static_assert(std::derived_from<From, To> || (std::derived_from<From, object> && std::derived_from<To, object>),
		              "Cannot cast between unrelated non-object types");

		if constexpr (std::derived_from<From, To>)
			return static_cast<To *>(ptr);
		else if constexpr (std::derived_from<From, object> && std::derived_from<To, object>)
		{
			const auto from_type = type_of(*ptr);
			const auto to_type = type_info::get<To>();
			if (from_type == to_type || from_type.inherits_from(to_type))
			{
				auto *obj = static_cast<take_const_t<object, From> *>(ptr);
				return static_cast<To *>(const_cast<object *>(obj));
			}
		}
		return nullptr;
	}

	namespace detail { struct dynamic_exception_tag {}; }

	/** Abstract base type used to create a dynamic exception derived from `object` and \a Base. */
	template<typename Base>
	struct REFLEX_VISIBLE dynamic_exception : public detail::dynamic_exception_tag, public object, public Base
	{
		constexpr dynamic_exception() = default;
		constexpr dynamic_exception(const dynamic_exception &) = default;
		constexpr dynamic_exception &operator=(const dynamic_exception &) = default;
		constexpr dynamic_exception(dynamic_exception &&) = default;
		constexpr dynamic_exception &operator=(dynamic_exception &&) = default;
		constexpr ~dynamic_exception() override = 0;

		template<typename... Args>
		constexpr dynamic_exception(Args &&...args) : Base(std::forward<Args>(args)...) {}
	};

	template<typename T>
	constexpr dynamic_exception<T>::~dynamic_exception() = default;

	/** Type initializer overload for dynamic object types. */
	template<std::derived_from<object> T> requires (!std::derived_from<T, detail::dynamic_exception_tag> && !std::same_as<T, object>)
	struct type_init<T> { void operator()(type_factory<T> factory) { factory.template add_parent<object>(); }};

	/** Type initializer overload for dynamic exceptions. */
	template<std::derived_from<object> T> requires std::derived_from<T, detail::dynamic_exception_tag>
	struct type_init<T>
	{
	private:
		template<typename U>
		static U get_dynamic_exception_base(const dynamic_exception<U> &);

		using dynamic_exception_base = decltype(get_dynamic_exception_base(std::declval<T>()));

	public:
		void operator()(type_factory<T> factory)
		{
			factory.template add_parent<object>();
			factory.template add_parent<dynamic_exception_base>();
			if constexpr (std::derived_from<T, std::exception> && std::same_as<dynamic_exception_base, std::exception>)
				factory.template add_parent<std::exception>();
			if constexpr (std::derived_from<T, std::logic_error> && std::same_as<dynamic_exception_base, std::logic_error>)
				factory.template add_parent<std::logic_error>();
			if constexpr (std::derived_from<T, std::runtime_error> && std::same_as<dynamic_exception_base, std::runtime_error>)
				factory.template add_parent<std::runtime_error>();
		}
	};

	/** Dynamic exception type thrown when the managed object of `any` cannot be copied. */
	class REFLEX_VISIBLE bad_any_copy final : public dynamic_exception<std::runtime_error>
	{
		[[nodiscard]] static std::string make_msg(const type_info &type)
		{
			std::string result;
			if (!type.valid())
				result.append("Invalid type");
			else
			{
				result.append("Type `");
				result.append(type.name());
				result.append(1, '`');
			}
			result.append(" is not copy-constructible");
			return result;
		}

		[[nodiscard]] reflex::type_info do_type_of() const final { return reflex::type_info::get<bad_any_copy>(); }

	public:
		bad_any_copy(const bad_any_copy &) = default;
		bad_any_copy &operator=(const bad_any_copy &) = default;
		bad_any_copy(bad_any_copy &&) = default;
		bad_any_copy &operator=(bad_any_copy &&) = default;

		/** Initializes the argument list exception with message \a msg and offending type \a type. */
		bad_any_copy(const char *msg, type_info type) : dynamic_exception<std::runtime_error>(msg), m_type(type) {}
		/** @copydoc bad_any_copy */
		bad_any_copy(const std::string &msg, type_info type) : dynamic_exception<std::runtime_error>(msg), m_type(type) {}

		/** Initializes the argument list exception with offending type \a type. */
		explicit bad_any_copy(type_info type) : bad_any_copy(make_msg(type), type) {}

		REFLEX_PUBLIC ~bad_any_copy() final;

		/** Returns type info of the offending (non-copyable) type. */
		[[nodiscard]] constexpr type_info type() const noexcept { return m_type; }

	private:
		type_info m_type;
	};

	/** Dynamic exception type thrown when the managed object of `any` cannot be cast to the desired type. */
	class REFLEX_VISIBLE bad_any_cast final : public dynamic_exception<std::runtime_error>
	{
		[[nodiscard]] static std::string make_msg(type_info from_type, type_info to_type)
		{
			std::string result;

			result.append("Managed object of ");
			if (!from_type.valid())
				result.append("an invalid type ");
			else
			{
				result.append("type `");
				result.append(from_type.name());
				result.append(1, '`');
			}

			result.append(" cannot be represented as or converted to ");
			if (!to_type.valid())
				result.append("an invalid type");
			else
			{
				result.append("type `");
				result.append(to_type.name());
				result.append(1, '`');
			}

			return result;
		}

		[[nodiscard]] reflex::type_info do_type_of() const final { return reflex::type_info::get<bad_any_cast>(); }

	public:
		bad_any_cast(const bad_any_cast &) = default;
		bad_any_cast &operator=(const bad_any_cast &) = default;
		bad_any_cast(bad_any_cast &&) = default;
		bad_any_cast &operator=(bad_any_cast &&) = default;

		/** Initializes the any cast exception from source type info and destination type info. */
		bad_any_cast(type_info from_type, type_info to_type) : dynamic_exception<std::runtime_error>(make_msg(from_type, to_type)), m_from_type(from_type), m_to_type(to_type) {}

		REFLEX_PUBLIC ~bad_any_cast() final;

		/** Returns type info of the converted-from type. */
		[[nodiscard]] constexpr type_info from_type() const noexcept { return m_from_type; }
		/** Returns type info of the converted-to type. */
		[[nodiscard]] constexpr type_info to_type() const noexcept { return m_to_type; }

	private:
		type_info m_from_type;
		type_info m_to_type;
	};

#ifdef REFLEX_HEADER_ONLY
	void any::throw_bad_any_cast(type_info from_type, type_info to_type) { throw bad_any_cast(from_type, to_type); }
	void any::throw_bad_any_copy(type_info type) { throw bad_any_copy(type); }

	bad_any_copy::~bad_any_copy() = default;
	bad_any_cast::~bad_any_cast() = default;
#endif
}

/** Convenience macro used to implement required `reflex::object` functions for type \a T. */
#define REFLEX_DEFINE_OBJECT(T) reflex::type_info do_type_of() const override { return reflex::type_info::get<T>(); }