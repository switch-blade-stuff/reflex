cmake_minimum_required(VERSION 3.20)
project(reflex-tests)
enable_testing()

function(make_test NAME FILE)
    set(TEST_PROJECT "${PROJECT_NAME}-${NAME}")
    add_executable(${TEST_PROJECT} ${FILE})
    add_test(NAME ${NAME} COMMAND "$<TARGET_FILE:${TEST_PROJECT}>")

    if (REFLEX_BUILD_STATIC)
        target_link_libraries(${TEST_PROJECT} PRIVATE reflex-static)
    elseif (REFLEX_BUILD_SHARED)
        target_link_libraries(${TEST_PROJECT} PRIVATE reflex-shared)
    endif ()

    # Enable max error reporting
    target_compile_options(${TEST_PROJECT} PUBLIC ${REFLEX_COMPILE_OPTIONS})
endfunction()

make_test(enum ${CMAKE_CURRENT_LIST_DIR}/test_enum.cpp)
make_test(object ${CMAKE_CURRENT_LIST_DIR}/test_object.cpp)