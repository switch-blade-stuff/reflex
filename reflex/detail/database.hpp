/*
 * Created by switchblade on 2023-03-14.
 */

#pragma once

#include "factory.hpp"

namespace reflex
{
	namespace detail
	{
		struct database_impl : shared_spinlock
		{
			static auto *local_ptr() noexcept
			{
				static database_impl value;
				return &value;
			}
			static auto &global_ptr() noexcept
			{
				static std::atomic<database_impl *> value = {};
				return value;
			}

			[[nodiscard]] REFLEX_PUBLIC static database_impl *instance() noexcept;
			REFLEX_PUBLIC static database_impl *instance(database_impl *ptr) noexcept;

			REFLEX_PUBLIC database_impl();
			REFLEX_PUBLIC database_impl(database_impl &&);
			REFLEX_PUBLIC ~database_impl();

			REFLEX_PUBLIC void reset();
			REFLEX_PUBLIC void reset(type_data &data);
			REFLEX_PUBLIC void reset(std::string_view name);

			REFLEX_PUBLIC type_data *find(std::string_view name);
			REFLEX_PUBLIC REFLEX_COLD type_data *insert(std::string_view name, const constant_type_data &data);

			/* stable_map is used to allow type_info to be a simple pointer to type_data. */
			tpp::stable_map<std::string, type_data, type_hash, type_eq> m_types;
		};

		template<typename T>
		type_data *data_factory(database_impl &db)
		{
			static const auto cdata =  constant_type_data{std::in_place_type<T>};
			static type_data *data = db.insert(type_name_v<T>, cdata);
			return data;
		}
	}

	template<typename... Args>
	argument_list::argument_list(type_pack_t<Args...> p) : argument_list(p, detail::database_impl::instance()) {}

	template<typename T>
	type_factory<T> type_info::reflect()
	{
		auto *db = detail::database_impl::instance();
		return {detail::data_factory<std::decay_t<T>>, *db};
	}

	type_info type_info::get(std::string_view name)
	{
		auto *db = detail::database_impl::instance();
		return {db->find(name), db};
	}
	template<typename T>
	type_info type_info::get()
	{
		auto *db = detail::database_impl::instance();
		return {detail::data_factory<std::decay_t<T>>, *db};
	}

	void type_info::reset(std::string_view name) { detail::database_impl::instance()->reset(name); }
	template<typename T>
	void type_info::reset() { reset(type_name_v<std::decay_t<T>>); }
	void type_info::reset() { detail::database_impl::instance()->reset(); }
}
