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

add_library(dosrecomp_decoder src/decoder/instruction_decoder.cpp)
add_library(dosrecomp::decoder ALIAS dosrecomp_decoder)
target_compile_features(dosrecomp_decoder PUBLIC cxx_std_23)
target_include_directories(dosrecomp_decoder PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_options(dosrecomp_decoder PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_library(dosrecomp_cfg src/cfg/cfg_builder.cpp)
add_library(dosrecomp::cfg ALIAS dosrecomp_cfg)
target_compile_features(dosrecomp_cfg PUBLIC cxx_std_23)
target_include_directories(dosrecomp_cfg PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(dosrecomp_cfg PUBLIC dosrecomp::decoder)
target_compile_options(dosrecomp_cfg PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_library(dosrecomp_ir src/ir/control_flow_ir.cpp src/ir/register_ssa.cpp)
add_library(dosrecomp::ir ALIAS dosrecomp_ir)
target_compile_features(dosrecomp_ir PUBLIC cxx_std_23)
target_include_directories(dosrecomp_ir PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(dosrecomp_ir PUBLIC dosrecomp::cfg)
target_compile_options(dosrecomp_ir PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(dosrecomp src/cli/main.cpp)
target_link_libraries(dosrecomp PRIVATE dosrecomp::loader dosrecomp::cfg dosrecomp::ir)
target_compile_options(dosrecomp PRIVATE -Wall -Wextra -Wpedantic -Werror)

if(DOSRECOMP_BUILD_TESTS)
    enable_testing()
    add_executable(loader_tests tests/loader_tests.cpp)
    target_link_libraries(loader_tests PRIVATE dosrecomp::loader)
    target_compile_options(loader_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME loader_tests COMMAND loader_tests)
    add_executable(decoder_tests tests/decoder_tests.cpp)
    target_link_libraries(decoder_tests PRIVATE dosrecomp::decoder)
    target_compile_options(decoder_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME decoder_tests COMMAND decoder_tests)
    add_executable(cfg_tests tests/cfg_tests.cpp)
    target_link_libraries(cfg_tests PRIVATE dosrecomp::cfg)
    target_compile_options(cfg_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME cfg_tests COMMAND cfg_tests)
    add_executable(ir_tests tests/ir_tests.cpp)
    target_link_libraries(ir_tests PRIVATE dosrecomp::ir)
    target_compile_options(ir_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME ir_tests COMMAND ir_tests)
    add_executable(ssa_tests tests/ssa_tests.cpp)
    target_link_libraries(ssa_tests PRIVATE dosrecomp::ir)
    target_compile_options(ssa_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    add_test(NAME ssa_tests COMMAND ssa_tests)
endif()
