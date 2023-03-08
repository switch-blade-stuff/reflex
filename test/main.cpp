/*
 * Created by switchblade on 2023-01-01.
 */

#ifdef REFLEX_HAS_SEKHMET
#undef REFLEX_HAS_SEKHMET
#endif

#include <reflex/type_name.hpp>

int main()
{
	return reflex::type_name<std::string_view>::value == "std::basic_string_view<char>";
}