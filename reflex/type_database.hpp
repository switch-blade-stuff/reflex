/*
 * Created by switchblade on 2023-03-20.
 */

#pragma once

#include "detail/database.hpp"
#include "type_info.hpp"

namespace reflex
{
	/** Opaque representation of the reflection database singleton. */
	class type_database : detail::database_impl
	{
		using impl_t = detail::database_impl;

		[[nodiscard]] static impl_t *impl_cast(type_database *ptr) noexcept { return static_cast<impl_t *>(ptr); }
		[[nodiscard]] static type_database *impl_cast(impl_t *ptr) noexcept { return static_cast<type_database *>(ptr); }

	public:
		/** Returns pointer to the global type database. */
		[[nodiscard]] static type_database *instance() noexcept { return impl_cast(impl_t::instance()); }
		/** Atomically replaces the global database pointer with \a ptr, and returns pointer to the old database.
		 * @warning Replacing database pointer while the old one is already in-use will lead to de-synchronization of reflected types,
		 * in turn potentially leading to unintended runtime bugs. */
		static type_database *instance(type_database *ptr) noexcept { return impl_cast(impl_t::instance(impl_cast(ptr))); }

	public:
		type_database() noexcept = default;
		~type_database() = default;

		type_database(const type_database &) = delete;
		type_database &operator=(const type_database &) = delete;
		type_database(type_database &&) = delete;
		type_database &operator=(type_database &&) = delete;
	};
}