
list(APPEND REFLEX_PUBLIC_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/api.hpp
        ${CMAKE_CURRENT_LIST_DIR}/define.hpp
        ${CMAKE_CURRENT_LIST_DIR}/utility.hpp
        ${CMAKE_CURRENT_LIST_DIR}/fwd.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_data.hpp
        ${CMAKE_CURRENT_LIST_DIR}/arg_info.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_info.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_domain.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_domain_impl.hpp
        ${CMAKE_CURRENT_LIST_DIR}/facet.hpp
        ${CMAKE_CURRENT_LIST_DIR}/any.hpp)

list(APPEND REFLEX_PRIVATE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/type_data.cpp
        ${CMAKE_CURRENT_LIST_DIR}/exceptions.cpp
        ${CMAKE_CURRENT_LIST_DIR}/type_domain.cpp)
