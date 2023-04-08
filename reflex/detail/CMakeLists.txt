
list(APPEND REFLEX_PUBLIC_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/api.hpp
        ${CMAKE_CURRENT_LIST_DIR}/define.hpp
        ${CMAKE_CURRENT_LIST_DIR}/spinlock.hpp
        ${CMAKE_CURRENT_LIST_DIR}/spinlock.ipp
        ${CMAKE_CURRENT_LIST_DIR}/fwd.hpp
        ${CMAKE_CURRENT_LIST_DIR}/data.hpp
        ${CMAKE_CURRENT_LIST_DIR}/info.hpp
        ${CMAKE_CURRENT_LIST_DIR}/info.ipp
        ${CMAKE_CURRENT_LIST_DIR}/database.hpp
        ${CMAKE_CURRENT_LIST_DIR}/database.ipp
        ${CMAKE_CURRENT_LIST_DIR}/factory.hpp
        ${CMAKE_CURRENT_LIST_DIR}/factory.ipp
        ${CMAKE_CURRENT_LIST_DIR}/object.hpp
        ${CMAKE_CURRENT_LIST_DIR}/query.hpp
        ${CMAKE_CURRENT_LIST_DIR}/query.ipp
        ${CMAKE_CURRENT_LIST_DIR}/facet.hpp
        ${CMAKE_CURRENT_LIST_DIR}/any.hpp
        ${CMAKE_CURRENT_LIST_DIR}/any.ipp
        ${CMAKE_CURRENT_LIST_DIR}/range_facet.hpp
        ${CMAKE_CURRENT_LIST_DIR}/tuple_facet.hpp
        ${CMAKE_CURRENT_LIST_DIR}/string_facet.hpp
        ${CMAKE_CURRENT_LIST_DIR}/pointer_facet.hpp)

list(APPEND REFLEX_PRIVATE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/spinlock.cpp
        ${CMAKE_CURRENT_LIST_DIR}/info.cpp
        ${CMAKE_CURRENT_LIST_DIR}/database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/factory.cpp
        ${CMAKE_CURRENT_LIST_DIR}/object.cpp
        ${CMAKE_CURRENT_LIST_DIR}/query.cpp
        ${CMAKE_CURRENT_LIST_DIR}/facet.cpp
        ${CMAKE_CURRENT_LIST_DIR}/any.cpp)
