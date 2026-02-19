cmake_minimum_required(VERSION 3.15)

project(ace-gb VERSION 0.0.1 LANGUAGES CXX)

set(EXE_NAME ace-gb)
set(TEST_NAME test)

option(BUILD_TESTS "Build tests" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(TOML_EXCEPTIONS=0)

set(SOURCE_FILES
        src/args.cpp
        src/args.hpp
    src/main.cpp
        src/interface.hpp
        src/interface.cpp
    src/cpu.h
    src/cpu.cpp
        src/emulator.hpp
        src/emulator.cpp
    src/decoder.h
    src/decoder.cpp
    src/instructions.h
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
        src/boot_rom_device.hpp
        src/boot_rom_device.cpp
        src/cart_device.hpp
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
        src/audio.hpp
    src/null_device.cpp
    src/null_device.h
    src/input_device.cpp
    src/input_device.h
    src/serial_device.cpp
    src/serial_device.h
        src/synced_device.hpp
    src/joypad.h
        src/cart_header.hpp
    src/memory_bank_controller.h
        src/CartInfo.h
    src/no_mbc.cpp
    src/no_mbc.h
    src/mbc1.cpp
    src/mbc1.h
    src/mbc2.cpp
    src/mbc2.h
    src/mbc3.cpp
    src/mbc3.h
    src/mbc5.cpp
    src/mbc5.h
    src/square_channel.cpp
    src/square_channel.h
    src/wave_channel.cpp
    src/wave_channel.h
    src/noise_channel.cpp
    src/noise_channel.h
        src/audio_channel.hpp
    src/config.h
    src/config.cpp
    src/recent_files.cpp
    src/recent_files.hpp
    src/assembly_viewer.cpp
    src/assembly_viewer.hpp
    src/applog.cpp
    src/applog.hpp
        src/mmu_device.hpp
)

set(ARGPARSE_BUILD_TESTS OFF)

set(SPDLOG_USE_STD_FORMAT ON)
add_subdirectory(external/spdlog)
add_subdirectory(external/tomlplusplus)
add_subdirectory(external/raylib)
add_subdirectory(external/argparse)
add_subdirectory(external/magic_enum)
add_subdirectory(external/json)

include(cmake/imgui.cmake)
include(cmake/rlimgui.cmake)
include(cmake/imgui_club.cmake)
include(cmake/nfd.cmake)


add_executable(${EXE_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_NAME} PRIVATE spdlog::spdlog)
target_link_libraries(${EXE_NAME} PRIVATE argparse::argparse)
target_link_libraries(${EXE_NAME} PRIVATE magic_enum::magic_enum)
target_link_libraries(${EXE_NAME} PRIVATE raylib)
target_link_libraries(${EXE_NAME} PRIVATE rlimgui)
target_link_libraries(${EXE_NAME} PRIVATE nfd)
target_link_libraries(${EXE_NAME} PRIVATE glfw)
target_link_libraries(${EXE_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${EXE_NAME} PRIVATE tomlplusplus::tomlplusplus)

target_include_directories(${EXE_NAME} PUBLIC ${RLIMGUI_INCLUDE_DIR})

if(BUILD_TESTS)
    set(TEST_FILES
        src/test.cpp
        src/cpu.h
        src/cpu.cpp
        src/decoder.h
        src/decoder.cpp
        src/instructions.h
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
            src/synced_device.hpp
    )

    add_executable(${TEST_NAME} ${TEST_FILES})

    target_link_libraries(${TEST_NAME} PRIVATE spdlog::spdlog)
    target_link_libraries(${TEST_NAME} PRIVATE argparse::argparse)
    target_link_libraries(${TEST_NAME} PRIVATE magic_enum::magic_enum)
    target_link_libraries(${TEST_NAME} PRIVATE nlohmann_json::nlohmann_json)
endif(BUILD_TESTS)
