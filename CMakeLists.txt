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
function(reflex_configure_target NAME TYPE)
    add_library(${NAME} ${TYPE})
    target_sources(${NAME} PUBLIC ${REFLEX_PUBLIC_SOURCES})
    target_sources(${NAME} INTERFACE ${REFLEX_INTERFACE_SOURCES})
    if (${TYPE} STREQUAL "INTERFACE")
        target_include_directories(${NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR})

        # Set C++ version
        target_compile_features(${NAME} INTERFACE cxx_std_20)

        # Link TPP interface target
        target_include_directories(${NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/external/tpp)
        target_link_libraries(${NAME} INTERFACE tpp)
    else ()
        target_sources(${NAME} PRIVATE ${REFLEX_PRIVATE_SOURCES})
        target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
        set_target_properties(${NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")

        # Set C++ version
        target_compile_features(${NAME} PUBLIC cxx_std_20)

        # Link TPP interface target
        target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/external/tpp)
        target_link_libraries(${NAME} PUBLIC tpp)

        # Set symbol visibility
        set_target_properties(${NAME} PROPERTIES C_VISIBILITY_PRESET hidden)
        set_target_properties(${NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
        set_target_properties(${NAME} PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

        # Set shared library definitions
        set_target_properties(${NAME} PROPERTIES DEFINE_SYMBOL "REFLEX_EXPORT")

        # Enable max error reporting
        if (MSVC)
            target_compile_options(${NAME} PRIVATE /W3 /WX)
        else ()
            target_compile_options(${NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unknown-pragmas -Wno-ignored-attributes)
        endif ()
    endif ()
endfunction()

if (${REFLEX_HEADER_ONLY})
    reflex_configure_target(${PROJECT_NAME}-header-only INTERFACE)
    target_compile_definitions(${PROJECT_NAME}-header-only INTERFACE REFLEX_HEADER_ONLY)
endif ()
if (${REFLEX_BUILD_SHARED})
    reflex_configure_target(${PROJECT_NAME}-shared SHARED)
endif ()
if (${REFLEX_BUILD_STATIC})
    reflex_configure_target(${PROJECT_NAME}-static STATIC)
    set_target_properties(${PROJECT_NAME}-static PROPERTIES PREFIX "lib")
    target_compile_definitions(${PROJECT_NAME}-static PUBLIC REFLEX_LIB_STATIC)
endif ()

# Add unit tests
option(REFLEX_TESTS "Enable unit tests" OFF)
if (${REFLEX_TESTS})
    include(${CMAKE_CURRENT_LIST_DIR}/test/CMakeLists.txt)
endif ()