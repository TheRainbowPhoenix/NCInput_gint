cmake_minimum_required(VERSION 3.15)
project(MyAddin)

include(GenerateG1A)
include(GenerateG3A)
include(GenerateHH3)
include(Fxconv)
find_package(Gint 2.9 REQUIRED)

set(SHARED_SOURCES
    src/shared/cinput.cpp
    src/shared/cdateinput.cpp
)

set(APPS "cinput_demo" "physchem" "app_template")

foreach(APP ${APPS})
    set(APP_SOURCES src/apps/${APP}/main.cpp ${SHARED_SOURCES})
    add_executable(${APP} ${APP_SOURCES})
    target_compile_options(${APP} PRIVATE -Wall -Wextra -Os -g -flto=auto -fno-exceptions)
    target_link_libraries(${APP} Gint::Gint stdc++)
    target_include_directories(${APP} PRIVATE src/shared)

    if("${FXSDK_PLATFORM_LONG}" STREQUAL fx9860G)
        generate_g1a(TARGET ${APP} OUTPUT "${APP}.g1a" NAME "${APP}")
    elseif("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
        generate_g3a(TARGET ${APP} OUTPUT "${APP}.g3a" NAME "${APP}")
    elseif("${FXSDK_PLATFORM_LONG}" STREQUAL fxCP)
        generate_hh3(TARGET ${APP} OUTPUT "${APP}.hh3")
    endif()
endforeach()
