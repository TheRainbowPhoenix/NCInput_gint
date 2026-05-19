cmake_minimum_required(VERSION 3.15)
project(cinput_simulator)

set(CMAKE_CXX_STANDARD 17)

# Detect if we are building for Emscripten
if(EMSCRIPTEN)
    set(IS_WEB TRUE)
elseif(WIN32)
    set(IS_WINDOWS TRUE)
    set(IS_NATIVE TRUE)
else()
    set(IS_NATIVE TRUE)
endif()

# Shared source files
set(SOURCES
    ../src/cinput.cpp
    ../src/cdateinput.cpp
    ../src/main.cpp
    src/simulator.cpp
)

include_directories(include ../src)

if(IS_NATIVE)
    find_package(SDL2 REQUIRED)
    add_executable(simulator ${SOURCES})
    target_link_libraries(simulator SDL2::SDL2)
    target_compile_definitions(simulator PRIVATE SIMULATOR_NATIVE)

    if(IS_WINDOWS)
        # On Windows, copy SDL2.dll next to the executable
        add_custom_command(TARGET simulator POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:SDL2::SDL2>/SDL2.dll"
            "$<TARGET_FILE_DIR:simulator>"
        )
    endif()
endif()

if(IS_WEB)
    add_executable(simulator_web ${SOURCES})
    set_target_properties(simulator_web PROPERTIES SUFFIX ".html")
    target_compile_options(simulator_web PRIVATE -sUSE_SDL=2)
    target_link_options(simulator_web PRIVATE
        -sUSE_SDL=2
        -sWASM=1
        -sASYNCIFY
        --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/shell.html"
    )
    target_compile_definitions(simulator_web PRIVATE SIMULATOR_WEB)
endif()
