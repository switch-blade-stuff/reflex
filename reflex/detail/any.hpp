/*
 * Created by switchblade on 2023-03-11.
 */

#pragma once

#include "type_data.hpp"

namespace reflex
{
	/** Type-erased generic object. */
	class any
	{
		friend class type_info;

		using local_storage = std::uint8_t[sizeof(std::intptr_t) * 2];
		struct ptr_storage
		{
			/* Separate deleter function is used to allow for user-specified allocators. */
			void (*deleter)(void *) = nullptr;
			void *data = nullptr;
		};

		template<typename T>
		constexpr static auto is_by_value = alignof(T) <= alignof(ptr_storage) && sizeof(T) <= sizeof(local_storage);

	public:
		~any() { destroy(); }

		/** Returns type of the managed object. */
		[[nodiscard]] constexpr type_info type() const noexcept;

		/** Checks if the `any` has a managed object (either value or reference). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_type == nullptr; }
		/** Checks if the managed object is const-qualified. Equivalent to `type().is_const()`. */
		[[nodiscard]] constexpr bool is_const() const noexcept { return m_flags & detail::IS_CONST; }
		/** Checks if the `any` contains a reference to an external object. Equivalent to `type().is_reference()`. */
		[[nodiscard]] constexpr bool is_reference() const noexcept { return !(m_flags & detail::IS_LOCAL); }

		/** Returns a void pointer to the managed object.
		 * @note If the managed object is const-qualified, returns `nullptr`. */
		[[nodiscard]] constexpr void *data() noexcept
		{
			if (m_flags & detail::IS_CONST) [[unlikely]] return nullptr;
			return (m_flags & detail::IS_LOCAL) ? m_local : m_external.data;
		}
		/** Returns a const void pointer to the managed object. */
		[[nodiscard]] constexpr const void *cdata() const noexcept
		{
			return (m_flags & detail::IS_LOCAL) ? m_local : m_external.data;
		}
		/** @copydoc cdata */
		[[nodiscard]] constexpr const void *data() const noexcept { return cdata(); }

	private:
		void destroy()
		{
			/* Bail if empty or non-owning. */
			if (empty() || !(m_flags & detail::IS_OWNED)) return;

			if (m_flags & detail::IS_LOCAL)
				m_type->dtor.destroy_in_place(m_local);
			else if (m_external.deleter == nullptr)
				m_type->dtor.destroy_delete(m_external.data);
			else
			{
				m_type->dtor.destroy_in_place(m_external.data);
				m_external.deleter(m_external.data);
			}
		}

		const detail::type_data *m_type = nullptr;
		union
		{
			local_storage m_local = {};
			ptr_storage m_external;
		};
		detail::type_flags m_flags;
	};
}