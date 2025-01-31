
set(imgui_SOURCES
    "${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/imgui_demo.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/imgui_draw.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/imgui_tables.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/imgui_widgets.cpp"
    "${CMAKE_SOURCE_DIR}/external/imgui/misc/cpp/imgui_stdlib.cpp"
)

add_library(imgui STATIC ${imgui_SOURCES})

include_directories(${CMAKE_SOURCE_DIR}/external/imgui)
