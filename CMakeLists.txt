cmake_minimum_required(VERSION 3.15)

project(ace-gb VERSION 0.0.1 LANGUAGES CXX)

set(EXE_NAME ace-gb)
set(TEST_NAME test)

option(BUILD_TESTS "Build tests" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(TOML_EXCEPTIONS=0)

set(SOURCE_FILES
        src/main.cpp
        src/interface.h
        src/interface.cpp
        src/cpu.h
        src/cpu.cpp
        src/emulator.h
        src/emulator.cpp
        src/decoder.h
        src/decoder.cpp
        src/instructions.h
        src/memory.h
        src/opcodes.h
        src/overloaded.h
        src/registers.h
        src/util.h
        src/mmu.h
        src/mmu.cpp
        src/ppu.h
        src/ppu.cpp
        src/io.h
        src/file.h
        src/file.cpp
        src/boot_rom_device.h
        src/boot_rom_device.cpp
        src/cart_device.h
        src/cart_device.cpp
        src/wram_device.cpp
        src/wram_device.h
        src/timer.cpp
        src/timer.h
        src/interrupt_device.cpp
        src/interrupt_device.h
        src/interrupt.h
        src/hram_device.cpp
        src/hram_device.h
        src/audio.cpp
        src/audio.h
        src/null_device.cpp
        src/null_device.h
        src/input_device.cpp
        src/input_device.h
        src/serial_device.cpp
        src/serial_device.h
        src/synced_device.h
        src/joypad.h
        src/cart_header.h
)

set(ARGPARSE_BUILD_TESTS OFF)

include(cmake/rlimgui.cmake)
include(cmake/imgui_club.cmake)
include(cmake/nfd.cmake)

find_package(spdlog CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)

add_executable(${EXE_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_NAME} PRIVATE spdlog::spdlog)
target_link_libraries(${EXE_NAME} PRIVATE argparse::argparse)
target_link_libraries(${EXE_NAME} PRIVATE magic_enum::magic_enum)
target_link_libraries(${EXE_NAME} PRIVATE raylib)
target_link_libraries(${EXE_NAME} PRIVATE rlimgui)
target_link_libraries(${EXE_NAME} PRIVATE nfd)
target_link_libraries(${EXE_NAME} PRIVATE glfw)
target_link_libraries(${EXE_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${EXE_NAME} PRIVATE tl::expected)

target_include_directories(${EXE_NAME} PUBLIC ${RLIMGUI_INCLUDE_DIR})

if(BUILD_TESTS)
    set(TEST_FILES
            src/test.cpp
            src/cpu.h
            src/cpu.cpp
            src/decoder.h
            src/decoder.cpp
            src/instructions.h
            src/memory.h
            src/opcodes.h
            src/overloaded.h
            src/registers.h
            src/util.h
            src/io.h
            src/mmu.h
            src/mmu.cpp
            src/interrupt.h
            src/interrupt_device.h
            src/interrupt_device.cpp
            src/synced_device.h
    )

    add_executable(${TEST_NAME} ${TEST_FILES})

    target_link_libraries(${TEST_NAME} PRIVATE spdlog::spdlog)
    target_link_libraries(${TEST_NAME} PRIVATE argparse::argparse)
    target_link_libraries(${TEST_NAME} PRIVATE magic_enum::magic_enum)
    target_link_libraries(${TEST_NAME} PRIVATE nlohmann_json::nlohmann_json)
    target_link_libraries(${TEST_NAME} PRIVATE tl::expected)
endif(BUILD_TESTS)
