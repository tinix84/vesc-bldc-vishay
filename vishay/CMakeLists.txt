cmake_minimum_required(VERSION 3.20)

project(vishay_vesc_support LANGUAGES C)

add_library(raik060_protocol STATIC
    src/raik060.c
)

target_include_directories(raik060_protocol
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(raik060_protocol PUBLIC c_std_11)

if(MSVC)
    target_compile_options(raik060_protocol PRIVATE /W4 /WX)
else()
    target_compile_options(raik060_protocol PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

include(CTest)

if(BUILD_TESTING)
    add_executable(test_raik060 tests/test_raik060.c)
    target_link_libraries(test_raik060 PRIVATE raik060_protocol)
    target_compile_features(test_raik060 PRIVATE c_std_11)

    if(MSVC)
        target_compile_options(test_raik060 PRIVATE /W4 /WX)
    else()
        target_compile_options(test_raik060 PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    add_test(NAME raik060_protocol COMMAND test_raik060)

    add_executable(test_vishay_hw tests/test_vishay_hw.c port/vesc/hwconf/hw_vishay_vesc.c)
    target_include_directories(test_vishay_hw PRIVATE tests/hal_stubs port/vesc/hwconf)
    target_link_libraries(test_vishay_hw PRIVATE raik060_protocol)
    if(MSVC)
        target_compile_options(test_vishay_hw PRIVATE /W4 /WX)
    else()
        target_compile_options(test_vishay_hw PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME vishay_hardware_configuration COMMAND test_vishay_hw)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../datatypes.h")
        set(VESC_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
    else()
        set(VESC_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/bldc")
    endif()
    add_executable(test_encoder_only_foc tests/test_encoder_only_foc.c port/vesc/encoder_only_foc.c)
    target_include_directories(test_encoder_only_foc PRIVATE tests/hal_stubs port/vesc)
    target_include_directories(test_encoder_only_foc SYSTEM PRIVATE "${VESC_SOURCE_ROOT}")
    target_compile_features(test_encoder_only_foc PRIVATE c_std_11)
    add_executable(test_raik060_vesc tests/test_raik060_vesc.c port/vesc/raik060_vesc.c)
    target_include_directories(test_raik060_vesc PRIVATE tests/hal_stubs port/vesc port/vesc/hwconf)
    target_link_libraries(test_raik060_vesc PRIVATE raik060_protocol)
    if(NOT MSVC)
        target_compile_options(test_encoder_only_foc PRIVATE -Wall -Wextra -Wpedantic -Werror)
        target_compile_options(test_raik060_vesc PRIVATE -Wall -Wextra -Wpedantic -Werror)
        target_link_libraries(test_encoder_only_foc PRIVATE m)
    endif()
    add_test(NAME encoder_only_foc_policy COMMAND test_encoder_only_foc)
    add_test(NAME raik060_encoder_health COMMAND test_raik060_vesc)
endif()

