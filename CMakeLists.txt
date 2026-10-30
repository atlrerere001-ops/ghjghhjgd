cmake_minimum_required(VERSION 3.20)
project(AegisRecovery LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Статическая линковка Runtime (/MT) - КРИТИЧНО для WinRE
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN)

include_directories(include)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "include/*.h"
)

add_executable(AegisRecovery ${SOURCES})

target_compile_options(AegisRecovery PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        /WX-
        /permissive-
        /EHsc
        /O2
        /Oi
        /Gy
        /GL
    >
)

target_link_options(AegisRecovery PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        /LTCG
        /SUBSYSTEM:CONSOLE
        /ENTRY:wmainCRTStartup
    >
)

target_link_libraries(AegisRecovery PRIVATE
    advapi32
    shell32
    ole32
    user32
    ntdll
)

set_target_properties(AegisRecovery PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
