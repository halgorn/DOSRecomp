cmake_minimum_required(VERSION 3.23)

project(DOSRecomp VERSION 0.1.0 LANGUAGES CXX)

option(DOSRECOMP_BUILD_TESTS "Build DOSRecomp tests" ON)

add_library(dosrecomp_loader
    src/loader/binary_loader.cpp
)
add_library(dosrecomp::loader ALIAS dosrecomp_loader)

target_compile_features(dosrecomp_loader PUBLIC cxx_std_23)
target_include_directories(dosrecomp_loader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_options(dosrecomp_loader PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(dosrecomp src/cli/main.cpp)
target_link_libraries(dosrecomp PRIVATE dosrecomp::loader)
target_compile_options(dosrecomp PRIVATE -Wall -Wextra -Wpedantic -Werror)

if(DOSRECOMP_BUILD_TESTS)
    enable_testing()
    add_executable(loader_tests tests/loader_tests.cpp)
    target_link_libraries(loader_tests PRIVATE dosrecomp::loader)
    target_compile_options(loader_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME loader_tests COMMAND loader_tests)
endif()

