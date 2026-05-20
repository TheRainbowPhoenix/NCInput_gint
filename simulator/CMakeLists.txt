cmake_minimum_required(VERSION 3.15)
project(cinput_simulator)

set(CMAKE_CXX_STANDARD 17)

if(EMSCRIPTEN)
    set(IS_WEB TRUE)
elseif(WIN32)
    set(IS_WINDOWS TRUE)
    set(IS_NATIVE TRUE)
else()
    set(IS_NATIVE TRUE)
endif()

set(SHARED_SOURCES
    ../src/shared/cinput.cpp
    ../src/shared/cdateinput.cpp
    ../src/shared/LazyFile.cpp
    src/simulator.cpp
)

include_directories(include ../src/shared ../src/apps/ced)

set(APPS "cinput_demo" "physchem" "app_template" "ced")

foreach(APP ${APPS})
    if("${APP}" STREQUAL "ced")
        set(APP_SOURCES
            ../src/apps/ced/main.cpp
            ../src/apps/ced/editor.cpp
            ../src/apps/ced/tokenizer.cpp
            ${SHARED_SOURCES}
        )
    else()
        set(APP_SOURCES ../src/apps/${APP}/main.cpp ${SHARED_SOURCES})
    endif()

    if(IS_NATIVE)
        find_package(SDL2 REQUIRED)
        add_executable(${APP}_sim ${APP_SOURCES})
        target_link_libraries(${APP}_sim SDL2::SDL2)
        target_compile_definitions(${APP}_sim PRIVATE SIMULATOR_NATIVE)
        if(IS_WINDOWS)
            add_custom_command(TARGET ${APP}_sim POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE_DIR:SDL2::SDL2>/SDL2.dll"
                "$<TARGET_FILE_DIR:${APP}_sim>"
            )
        endif()
    endif()

    if(IS_WEB)
        add_executable(${APP}_web ${APP_SOURCES})
        set_target_properties(${APP}_web PROPERTIES SUFFIX ".html")
        target_compile_options(${APP}_web PRIVATE -sUSE_SDL=2)
        target_link_options(${APP}_web PRIVATE
            -sUSE_SDL=2
            -sWASM=1
            -sASYNCIFY
            --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/shell.html"
        )
        target_compile_definitions(${APP}_web PRIVATE SIMULATOR_WEB)
    endif()
endforeach()
