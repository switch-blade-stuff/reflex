cmake_minimum_required(VERSION 3.20)
project(reflex-tests)
enable_testing()

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/main.cpp)
if (REFLEX_BUILD_SHARED)
    target_link_libraries(${PROJECT_NAME} PRIVATE reflex-shared)
elseif (REFLEX_BUILD_STATIC)
    target_link_libraries(${PROJECT_NAME} PRIVATE reflex-static)
endif ()

# Enable max error reporting
target_compile_options(${PROJECT_NAME} PUBLIC ${REFLEX_COMPILE_OPTIONS})