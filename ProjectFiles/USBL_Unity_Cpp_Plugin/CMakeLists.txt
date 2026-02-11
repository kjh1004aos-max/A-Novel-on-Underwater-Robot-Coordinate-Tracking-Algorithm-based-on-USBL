
cmake_minimum_required(VERSION 3.16)
project(USBLUnityPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(USBLUnityPlugin SHARED
    src/usbl_plugin.cpp
    src/serial_port.cpp
    src/line_framer.cpp
    src/parser.cpp
    src/tracker.cpp
    src/controller.cpp
)

target_include_directories(USBLUnityPlugin PUBLIC src)

# Reduce symbol export noise on *nix
if (UNIX AND NOT APPLE)
    target_compile_options(USBLUnityPlugin PRIVATE -fvisibility=hidden)
endif()

# Windows specifics
if (WIN32)
    target_compile_definitions(USBLUnityPlugin PRIVATE USBL_BUILD_DLL=1)
endif()
