/*
 * Created by switchblade on 2023-01-01.
 */

#ifndef REFLEX_HAS_SEKHMET
#define REFLEX_HAS_SEKHMET
#endif

#include <type_name.hpp>

#include <vector>
#include <cstdio>

int main()
{
	printf("\"%s\"\n", sek::type_name<int>::value.data());
	printf("\"%s\"\n", sek::type_name<std::string_view>::value.data());
	printf("\"%s\"\n", sek::type_name<std::vector<int>>::value.data());
}