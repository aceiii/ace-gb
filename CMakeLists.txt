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
        src/applog.cpp
        src/applog.hpp
        src/args.cpp
        src/args.hpp
        src/assembly_viewer.cpp
        src/assembly_viewer.hpp
        src/audio_channel.hpp
        src/audio.cpp
        src/audio.hpp
        src/boot_rom_device.cpp
        src/boot_rom_device.hpp
        src/cart_device.cpp
        src/cart_device.hpp
        src/cart_header.hpp
        src/cart_info.hpp
        src/config.cpp
        src/config.hpp
        src/cpu.cpp
        src/cpu.hpp
        src/decoder.cpp
        src/decoder.hpp
        src/emulator.cpp
        src/emulator.hpp
        src/error_messages.cpp
        src/error_messages.hpp
        src/file.cpp
        src/file.hpp
        src/hram_device.cpp
        src/hram_device.hpp
        src/input_device.cpp
        src/input_device.hpp
        src/instructions.hpp
        src/interface.cpp
        src/interface.hpp
        src/interrupt_device.cpp
        src/interrupt_device.hpp
        src/interrupt.hpp
        src/io.hpp
        src/joypad.hpp
        src/main.cpp
        src/mbc1.cpp
        src/mbc1.hpp
        src/mbc2.cpp
        src/mbc2.hpp
        src/mbc3.cpp
        src/mbc3.hpp
        src/mbc5.cpp
        src/mbc5.hpp
        src/memory_bank_controller.hpp
        src/mmu_device.hpp
        src/mmu.cpp
        src/mmu.hpp
        src/no_mbc.cpp
        src/no_mbc.hpp
        src/noise_channel.cpp
        src/noise_channel.h
        src/null_device.cpp
        src/null_device.h
        src/opcodes.hpp
        src/overloaded.h
        src/ppu.cpp
        src/ppu.hpp
        src/recent_files.cpp
        src/recent_files.hpp
        src/registers.hpp
        src/serial_device.cpp
        src/serial_device.hpp
        src/square_channel.cpp
        src/square_channel.hpp
        src/synced_device.hpp
        src/timer.cpp
        src/timer.hpp
        src/util.hpp
        src/wave_channel.cpp
        src/wave_channel.hpp
        src/wram_device.cpp
        src/wram_device.hpp
        src/types.hpp
)

set(ARGPARSE_BUILD_TESTS OFF)

if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    option(TRACY_ENABLE "" OFF)
    option(TRACY_ON_DEMAND "" OFF)
else()
    option(TRACY_ENABLE "" ON)
    option(TRACY_ON_DEMAND "" ON)
endif()

set(SPDLOG_USE_STD_FORMAT ON)
add_subdirectory(external/spdlog)
add_subdirectory(external/tomlplusplus)
add_subdirectory(external/raylib)
add_subdirectory(external/argparse)
add_subdirectory(external/magic_enum)
add_subdirectory(external/json)
add_subdirectory(external/tracy)

include(cmake/imgui.cmake)
include(cmake/rlimgui.cmake)
include(cmake/imgui_club.cmake)

add_executable(${EXE_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_NAME} PRIVATE spdlog::spdlog)
target_link_libraries(${EXE_NAME} PRIVATE argparse::argparse)
target_link_libraries(${EXE_NAME} PRIVATE magic_enum::magic_enum)
target_link_libraries(${EXE_NAME} PRIVATE raylib)
target_link_libraries(${EXE_NAME} PRIVATE rlimgui)
target_link_libraries(${EXE_NAME} PRIVATE glfw)
target_link_libraries(${EXE_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${EXE_NAME} PRIVATE tomlplusplus::tomlplusplus)
target_link_libraries(${EXE_NAME} PRIVATE TracyClient)

target_include_directories(${EXE_NAME} PRIVATE ${RLIMGUI_INCLUDE_DIR})

if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    target_include_directories(${EXE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/emscripten-browser-file)

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    target_link_options(${EXE_NAME} PUBLIC
        "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
        "-sSTACK_SIZE=8MB"
        "-sASYNCIFY"
        "-sTOTAL_STACK=64MB"
        "-sINITIAL_MEMORY=128MB"
        "-sEXPORTED_RUNTIME_METHODS=[ccall,HEAPU8,HEAPF32]"
        "-sEXPORTED_FUNCTIONS=[_main,_malloc,_free]"
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/resources@resources"
        "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/roms/dmg_boot.bin@boot.bin"
    )
else()
    include(cmake/nfd.cmake)
    target_link_libraries(${EXE_NAME} PRIVATE nfd)
endif()

if(BUILD_TESTS)
    set(TEST_FILES
            src/test.cpp
            src/cpu.hpp
            src/cpu.cpp
            src/decoder.hpp
            src/decoder.cpp
            src/instructions.hpp
            src/opcodes.hpp
            src/overloaded.h
            src/registers.hpp
            src/util.hpp
            src/io.hpp
            src/mmu.hpp
            src/mmu.cpp
            src/interrupt.hpp
            src/interrupt_device.hpp
            src/interrupt_device.cpp
            src/synced_device.hpp
    )

    add_executable(${TEST_NAME} ${TEST_FILES})

    target_link_libraries(${TEST_NAME} PRIVATE spdlog::spdlog)
    target_link_libraries(${TEST_NAME} PRIVATE argparse::argparse)
    target_link_libraries(${TEST_NAME} PRIVATE magic_enum::magic_enum)
    target_link_libraries(${TEST_NAME} PRIVATE nlohmann_json::nlohmann_json)
endif(BUILD_TESTS)
