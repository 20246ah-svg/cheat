cmake_minimum_required(VERSION 3.15)

project(cheat VERSION 1.0.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This project targets the Windows version of ClientMod.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR "Build with Visual Studio 2019 or newer (MSVC).")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Select the Win32 generator platform: cmake -A Win32 ...")
endif()

option(CHEAT_DIAGNOSTICS
    "Show diagnostic MessageBoxes and draw a fixed red PaintTraverse test square"
    ON
)

add_library(cheat SHARED
    main.cpp
    engine.cpp
    engine.h
    entity.h
    esp.cpp
    esp.h
    hooks.cpp
    hooks.h
    vector.h
)

target_compile_features(cheat PRIVATE cxx_std_17)
target_compile_definitions(cheat PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
if(CHEAT_DIAGNOSTICS)
    target_compile_definitions(cheat PRIVATE CHEAT_DIAGNOSTICS=1)
endif()
target_compile_options(cheat PRIVATE /W4 /permissive- /EHsc /utf-8)
target_link_libraries(cheat PRIVATE user32)

set_property(TARGET cheat PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set_target_properties(cheat PROPERTIES
    OUTPUT_NAME "cheat"
    PREFIX ""
)
