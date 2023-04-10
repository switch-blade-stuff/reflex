
include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)

list(APPEND REFLEX_PUBLIC_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/utility.hpp
        ${CMAKE_CURRENT_LIST_DIR}/const_string.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_name.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_info.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_database.hpp)
