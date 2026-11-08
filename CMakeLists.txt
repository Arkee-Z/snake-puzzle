cmake_minimum_required(VERSION 3.20)

project(snake_puzzle VERSION 1.1.0 LANGUAGES C)

set(SP_RELEASE_VERSION "${PROJECT_VERSION}" CACHE STRING "Version reported by --version")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version" FORCE)
endif()

option(SP_USE_SYSTEM_RAYLIB "Use an installed raylib instead of FetchContent" OFF)
option(SP_BUILD_TESTS "Build the headless C test suite" ON)

if(SP_USE_SYSTEM_RAYLIB)
    find_package(raylib 6.0 QUIET)
    if(NOT raylib_FOUND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib>=6.0)
        add_library(raylib ALIAS PkgConfig::RAYLIB)
    endif()
else()
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(PLATFORM Desktop CACHE STRING "" FORCE)
    set(USE_EXTERNAL_GLFW OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

file(GLOB_RECURSE SP_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/entity/*.c"
)
file(GLOB_RECURSE SP_GEN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gen/*.c"
)
set(SP_APP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/app.c")
set(SP_PLATFORM_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/platform.c")
set(SP_UI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/render_cli.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/render_raylib.c"
)

add_executable(snake_puzzle
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
    ${SP_APP_SOURCE}
    ${SP_PLATFORM_SOURCE}
    ${SP_CORE_SOURCES}
    ${SP_GEN_SOURCES}
    ${SP_UI_SOURCES}
)

target_include_directories(snake_puzzle PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_definitions(snake_puzzle PRIVATE
    SP_DEFAULT_ATOMS_DIR="assets/atoms"
    SP_VERSION="${SP_RELEASE_VERSION}"
)
if(NOT WIN32)
    target_compile_definitions(snake_puzzle PRIVATE _POSIX_C_SOURCE=200809L)
endif()

if(MSVC)
    target_compile_options(snake_puzzle PRIVATE /W4 /permissive-)
    target_compile_definitions(snake_puzzle PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(snake_puzzle PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(snake_puzzle PRIVATE raylib)
if(NOT WIN32)
    target_link_libraries(snake_puzzle PRIVATE m)
endif()
if(MINGW)
    target_link_options(snake_puzzle PRIVATE -static -static-libgcc)
endif()

if(APPLE)
    set_target_properties(snake_puzzle PROPERTIES
        MACOSX_RPATH OFF
        BUILD_WITH_INSTALL_RPATH OFF
    )
endif()

if(SP_BUILD_TESTS)
    file(GLOB SP_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c"
    )
    add_executable(snake_puzzle_tests
        ${SP_TEST_SOURCES}
        ${SP_APP_SOURCE}
        ${SP_PLATFORM_SOURCE}
        ${SP_CORE_SOURCES}
        ${SP_GEN_SOURCES}
    )
    target_include_directories(snake_puzzle_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )
    target_compile_definitions(snake_puzzle_tests PRIVATE
        SP_DEFAULT_ATOMS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/atoms"
    )
    if(NOT WIN32)
        target_compile_definitions(snake_puzzle_tests PRIVATE _POSIX_C_SOURCE=200809L)
        target_link_libraries(snake_puzzle_tests PRIVATE m)
    endif()
    enable_testing()
    add_test(NAME snake_puzzle_core
             COMMAND snake_puzzle_tests
             WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

install(TARGETS snake_puzzle RUNTIME DESTINATION bin)
install(DIRECTORY assets/atoms DESTINATION share/snake-puzzle/assets)
install(FILES README.md LICENSE THIRD_PARTY_NOTICES.md DESTINATION share/snake-puzzle)
install(DIRECTORY licenses DESTINATION share/snake-puzzle)
