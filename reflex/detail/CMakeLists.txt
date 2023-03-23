
list(APPEND REFLEX_PUBLIC_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/api.hpp
        ${CMAKE_CURRENT_LIST_DIR}/define.hpp
        ${CMAKE_CURRENT_LIST_DIR}/utility.hpp
        ${CMAKE_CURRENT_LIST_DIR}/fwd.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_data.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_info.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_info.ipp
        ${CMAKE_CURRENT_LIST_DIR}/database.hpp
        ${CMAKE_CURRENT_LIST_DIR}/database.ipp
        ${CMAKE_CURRENT_LIST_DIR}/facet.hpp
        ${CMAKE_CURRENT_LIST_DIR}/any.hpp
        ${CMAKE_CURRENT_LIST_DIR}/any.ipp)

list(APPEND REFLEX_PRIVATE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/type_info.cpp
        ${CMAKE_CURRENT_LIST_DIR}/database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/any.cpp)
