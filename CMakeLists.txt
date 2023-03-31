cmake_minimum_required(VERSION 3.20)
project(reflex)

# Library options
option(REFLEX_NO_ARITHMETIC "Disables default conversions for arithmetic types" OFF)
if (${REFLEX_NO_ARITHMETIC})
    add_compile_definitions(REFLEX_NO_ARITHMETIC)
endif ()

option(REFLEX_HEADER_ONLY "Toggles the header-only library target" ON)
option(REFLEX_BUILD_SHARED "Toggles build of shared library target" ON)
option(REFLEX_BUILD_STATIC "Toggles build of static library target" ON)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib CACHE STRING "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "")

# Include library source subdirectory
set(REFLEX_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/reflex)
include(${REFLEX_SOURCE_DIR}/CMakeLists.txt)

# Add TPP subproject
set(TPP_USE_MODULES OFF CACHE BOOL "Disable modules")
set(TPP_STL_HASH_ALL ON CACHE BOOL "Enable hash implementation for STL types")
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/tpp)

# Add shared & static library targets
if (MSVC)
    set(REFLEX_COMPILE_OPTIONS /W3 /WX)
else ()
    set(REFLEX_COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Werror -Wno-unknown-pragmas -Wno-ignored-attributes)
endif ()

function(reflex_configure_target NAME TYPE)
    add_library(${NAME} ${TYPE})
    target_sources(${NAME} PUBLIC ${REFLEX_PUBLIC_SOURCES})
    target_sources(${NAME} INTERFACE ${REFLEX_INTERFACE_SOURCES})
    if (${TYPE} STREQUAL "INTERFACE")
        # Set C++ version
        target_compile_features(${NAME} INTERFACE cxx_std_20)

        # Include directories
        target_include_directories(${NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})
        target_include_directories(${NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/external/tpp)

        # Link TPP target
        target_link_libraries(${NAME} INTERFACE tpp)

        # Set definitions
        target_compile_definitions(${PROJECT_NAME}-interface INTERFACE REFLEX_HEADER_ONLY)
    else ()
        target_sources(${NAME} PRIVATE ${REFLEX_PRIVATE_SOURCES})
        set_target_properties(${NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")

        # Set C++ version
        target_compile_features(${NAME} PUBLIC cxx_std_20)

        # Include directories
        target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
        target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/external/tpp)

        # Link TPP target
        target_link_libraries(${NAME} PUBLIC tpp)

        # Set symbol visibility
        set_target_properties(${NAME} PROPERTIES C_VISIBILITY_PRESET hidden)
        set_target_properties(${NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
        set_target_properties(${NAME} PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

        # Set definitions
        set_target_properties(${NAME} PROPERTIES DEFINE_SYMBOL "REFLEX_EXPORT")
        target_compile_definitions(${NAME} PUBLIC "REFLEX_LIB_${TYPE}")

        # Enable max error reporting
        target_compile_options(${NAME} PRIVATE ${REFLEX_COMPILE_OPTIONS})
    endif ()
endfunction()

if (${REFLEX_BUILD_STATIC})
    reflex_configure_target(${PROJECT_NAME}-static STATIC)
    set_target_properties(${PROJECT_NAME}-static PROPERTIES PREFIX "lib")
endif ()
if (${REFLEX_BUILD_SHARED})
    reflex_configure_target(${PROJECT_NAME}-shared SHARED)
endif ()
if (${REFLEX_HEADER_ONLY})
    reflex_configure_target(${PROJECT_NAME}-interface INTERFACE)
endif ()

# Add unit tests
option(REFLEX_TESTS "Enable unit tests" OFF)
if (${REFLEX_TESTS})
    include(${CMAKE_CURRENT_LIST_DIR}/test/CMakeLists.txt)
endif ()