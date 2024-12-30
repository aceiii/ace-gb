cmake_minimum_required(VERSION 3.15)

project(ace-gb VERSION 0.0.1 LANGUAGES CXX)

set(EXE_NAME ace-gb)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(TOML_EXCEPTIONS=0)

set(SOURCE_FILES
    src/main.cpp
    src/interface.cpp
    src/cpu.cpp
    src/emulator.cpp
    src/decoder.cpp
)

set(ARGPARSE_BUILD_TESTS OFF)

include(cmake/rlimgui.cmake)
include(cmake/imgui_club.cmake)
include(cmake/nfd.cmake)

find_package(spdlog CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(${EXE_NAME} ${SOURCE_FILES})

target_link_libraries(${EXE_NAME} PRIVATE spdlog::spdlog)
target_link_libraries(${EXE_NAME} PRIVATE argparse::argparse)
target_link_libraries(${EXE_NAME} PRIVATE magic_enum::magic_enum)
target_link_libraries(${EXE_NAME} PRIVATE raylib)
target_link_libraries(${EXE_NAME} PRIVATE rlimgui)
target_link_libraries(${EXE_NAME} PRIVATE nfd)
target_link_libraries(${EXE_NAME} PRIVATE glfw)

target_include_directories(${EXE_NAME} PUBLIC ${RLIMGUI_INCLUDE_DIR})
