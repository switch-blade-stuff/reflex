/*
 * Created by switchblade on 2023-03-31.
 */

#include <reflex/type_database.hpp>

#include "common.hpp"

int main()
{
	const auto old_db = reflex::type_database::instance();

	reflex::type_database db;
	TEST_ASSERT(reflex::type_database::instance(&db) == old_db);
	TEST_ASSERT(reflex::type_database::instance() == &db);
}