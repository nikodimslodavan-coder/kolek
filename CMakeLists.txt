cmake_minimum_required(VERSION 3.16)
project(ApartmentsDiamond VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 CONFIG QUIET)
if (NOT SDL2_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
endif()

add_executable(mansion src/main.cpp)
target_include_directories(mansion PRIVATE src)

if (TARGET SDL2::SDL2)
    # SDL2main обязательно раньше SDL2: иначе single-pass линковщик MinGW
    # не найдёт SDL_* символы, которые нужны SDL_windows_main.c.
    if (TARGET SDL2::SDL2main)
        target_link_libraries(mansion PRIVATE SDL2::SDL2main)
    endif()
    target_link_libraries(mansion PRIVATE SDL2::SDL2)
elseif (TARGET PkgConfig::SDL2)
    target_link_libraries(mansion PRIVATE PkgConfig::SDL2)
else()
    target_include_directories(mansion PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(mansion PRIVATE ${SDL2_LIBRARIES})
endif()

if (MSVC)
    target_compile_options(mansion PRIVATE /utf-8 /W4)
else()
    target_compile_options(mansion PRIVATE -Wall -Wextra)
endif()

# Подложить SDL2.dll рядом с exe (Windows/MinGW)
if (WIN32)
    find_file(SDL2_RUNTIME_DLL
        NAMES SDL2.dll
        HINTS
            "$ENV{MSYSTEM_PREFIX}/bin"
            "${SDL2_DIR}/../../../bin"
            "${SDL2_DIR}/../../bin"
    )
    if (SDL2_RUNTIME_DLL)
        add_custom_command(TARGET mansion POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SDL2_RUNTIME_DLL}" "$<TARGET_FILE_DIR:mansion>")
    endif()
endif()
