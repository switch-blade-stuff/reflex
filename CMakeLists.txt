cmake_minimum_required(VERSION 3.20)
project(reflex)

# Library options
option(REFLEX_HAS_SEKHMET "Enables sekhmet-specific features & places the library into `sek` namespace" OFF)
if (${REFLEX_HAS_SEKHMET})
    add_compile_definitions(REFLEX_HAS_SEKHMET)
endif ()

option(REFLEX_HEADER_ONLY "Toggles whether the library is header-only" ON)
if (${REFLEX_HEADER_ONLY})
    add_compile_definitions(REFLEX_HEADER_ONLY)
endif ()

option(REFLEX_BUILD_OBJECT "Toggles build of object library target" ON)
option(REFLEX_BUILD_SHARED "Toggles build of shared library target" ON)
option(REFLEX_BUILD_STATIC "Toggles build of static library target" ON)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib CACHE STRING "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "")

# Include library source subdirectory
set(REFLEX_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/reflex)
include(${REFLEX_SOURCE_DIR}/CMakeLists.txt)

# Add project targets & include directories
if (MSVC)
    set(REFLEX_COMPILE_OPTIONS /W3 /WX)
else ()
    set(REFLEX_COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Werror -Wno-unknown-pragmas -Wno-ignored-attributes)
endif ()
if (REFLEX_BUILD_OBJECT)
    add_library(${PROJECT_NAME}-object OBJECT)
    target_sources(${PROJECT_NAME}-object PUBLIC ${REFLEX_PUBLIC_SOURCES})
    target_sources(${PROJECT_NAME}-object PRIVATE ${REFLEX_PRIVATE_SOURCES})
    target_sources(${PROJECT_NAME}-object INTERFACE ${REFLEX_INTERFACE_SOURCES})
    target_include_directories(${PROJECT_NAME}-object PUBLIC ${CMAKE_CURRENT_LIST_DIR})
    set_target_properties(${PROJECT_NAME}-object PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")

    # Set C++ version
    target_compile_features(${PROJECT_NAME}-object PUBLIC cxx_std_20)

    # Set symbol visibility
    set_target_properties(${PROJECT_NAME}-object PROPERTIES C_VISIBILITY_PRESET hidden)
    set_target_properties(${PROJECT_NAME}-object PROPERTIES CXX_VISIBILITY_PRESET hidden)
    set_target_properties(${PROJECT_NAME}-object PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

    # Set shared library definitions
    set_target_properties(${PROJECT_NAME}-object PROPERTIES DEFINE_SYMBOL "REFLEX_EXPORT")
    target_compile_definitions(${PROJECT_NAME}-object PUBLIC REFLEX_LIB_SHARED)

    # Enable max error reporting
    target_compile_options(${PROJECT_NAME}-object PRIVATE ${REFLEX_COMPILE_OPTIONS})
endif ()

if (REFLEX_BUILD_SHARED)
    add_library(${PROJECT_NAME}-shared SHARED)
    target_sources(${PROJECT_NAME}-shared PUBLIC ${REFLEX_PUBLIC_SOURCES})
    target_sources(${PROJECT_NAME}-shared PRIVATE ${REFLEX_PRIVATE_SOURCES})
    target_sources(${PROJECT_NAME}-shared INTERFACE ${REFLEX_INTERFACE_SOURCES})
    target_include_directories(${PROJECT_NAME}-shared PUBLIC ${CMAKE_CURRENT_LIST_DIR})
    set_target_properties(${PROJECT_NAME}-shared PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")

    # Set C++ version
    target_compile_features(${PROJECT_NAME}-shared PUBLIC cxx_std_20)

    # Set symbol visibility
    set_target_properties(${PROJECT_NAME}-shared PROPERTIES C_VISIBILITY_PRESET hidden)
    set_target_properties(${PROJECT_NAME}-shared PROPERTIES CXX_VISIBILITY_PRESET hidden)
    set_target_properties(${PROJECT_NAME}-shared PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

    # Set shared library definitions
    set_target_properties(${PROJECT_NAME}-shared PROPERTIES DEFINE_SYMBOL "REFLEX_EXPORT")
    target_compile_definitions(${PROJECT_NAME}-shared PUBLIC REFLEX_LIB_SHARED)

    # Enable max error reporting
    target_compile_options(${PROJECT_NAME}-shared PRIVATE ${REFLEX_COMPILE_OPTIONS})
endif ()

if (REFLEX_BUILD_STATIC)
    add_library(${PROJECT_NAME}-static STATIC)
    target_sources(${PROJECT_NAME}-static PUBLIC ${REFLEX_PUBLIC_SOURCES})
    target_sources(${PROJECT_NAME}-static PRIVATE ${REFLEX_PRIVATE_SOURCES})
    target_sources(${PROJECT_NAME}-static INTERFACE ${REFLEX_INTERFACE_SOURCES})
    target_include_directories(${PROJECT_NAME}-static PUBLIC ${CMAKE_CURRENT_LIST_DIR})
    set_target_properties(${PROJECT_NAME}-static PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")

    # Set C++ version
    target_compile_features(${PROJECT_NAME}-static PUBLIC cxx_std_20)

    # Set symbol visibility
    set_target_properties(${PROJECT_NAME}-static PROPERTIES C_VISIBILITY_PRESET hidden)
    set_target_properties(${PROJECT_NAME}-static PROPERTIES CXX_VISIBILITY_PRESET hidden)
    set_target_properties(${PROJECT_NAME}-static PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

    # Set shared library definitions
    set_target_properties(${PROJECT_NAME}-static PROPERTIES DEFINE_SYMBOL "REFLEX_EXPORT")
    target_compile_definitions(${PROJECT_NAME}-static PUBLIC REFLEX_LIB_STATIC)

    # Enable max error reporting
    target_compile_options(${PROJECT_NAME}-static PRIVATE ${REFLEX_COMPILE_OPTIONS})
endif ()

# Add unit tests
option(REFLEX_TESTS "Enable unit tests" OFF)
if (${REFLEX_TESTS})
    include(${CMAKE_CURRENT_LIST_DIR}/test/CMakeLists.txt)
endif ()