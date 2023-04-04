/*
 * Created by switchblade on 2023-03-31.
 */

#include <reflex/type_database.hpp>

#include "common.hpp"

int main()
{
	const auto old_db = reflex::type_database::instance();
	TEST_ASSERT(!reflex::type_info::get("int").valid());

	reflex::type_info::reflect<int>();
	TEST_ASSERT(reflex::type_info::get("int").valid());

	auto new_db = reflex::type_database{std::move(*old_db)};
	TEST_ASSERT(reflex::type_database::instance(&new_db) == old_db);
	TEST_ASSERT(reflex::type_database::instance() == &new_db);
	TEST_ASSERT(reflex::type_info::get("int").valid());
}