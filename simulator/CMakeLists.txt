cmake_minimum_required(VERSION 3.15)
project(Simulator)

set(CMAKE_CXX_STANDARD 17)

# Find SDL2
if(EMSCRIPTEN)
    add_compile_options("-sUSE_SDL=2")
    add_link_options("-sUSE_SDL=2")
    set(SDL2_LIBRARIES "")
else()
    find_package(SDL2 REQUIRED)
    set(SDL2_LIBRARIES SDL2::SDL2)
endif()

include_directories(include)
include_directories(../src/shared)

set(SHARED_SOURCES
    ../src/shared/cinput.cpp
    ../src/shared/cdateinput.cpp
    ../src/shared/LazyFile.cpp
    ../src/shared/mcs.cpp
)

set(APPS "cinput_demo" "physchem" "app_template" "ced" "cptime" "fm" "mcsbrowse")

foreach(APP ${APPS})
    if("${APP}" STREQUAL "ced")
        set(APP_SOURCES
            ../src/apps/ced/main.cpp
            ../src/apps/ced/editor.cpp
            ../src/apps/ced/tokenizer.cpp
            ${SHARED_SOURCES}
            src/simulator.cpp
        )
    elseif("${APP}" STREQUAL "mcsbrowse")
        set(APP_SOURCES
            ../src/apps/mcsbrowse/main.cpp
            ${SHARED_SOURCES}
            src/simulator.cpp
        )
    elseif("${APP}" STREQUAL "fm")
        set(APP_SOURCES
            ../src/apps/fm/main.cpp
            ../src/apps/fm/fm.cpp
            ${SHARED_SOURCES}
            src/simulator.cpp
        )
    else()
        set(APP_SOURCES ../src/apps/${APP}/main.cpp ${SHARED_SOURCES} src/simulator.cpp)
    endif()

    if(EMSCRIPTEN)
        add_executable(${APP}_web ${APP_SOURCES})
        target_link_libraries(${APP}_web ${SDL2_LIBRARIES})
        set_target_properties(${APP}_web PROPERTIES SUFFIX ".html")
        target_link_options(${APP}_web PRIVATE "-sASYNCIFY" "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/shell.html")
    else()
        add_executable(${APP}_sim ${APP_SOURCES})
        target_link_libraries(${APP}_sim ${SDL2_LIBRARIES})
    endif()
endforeach()
