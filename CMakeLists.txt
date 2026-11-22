cmake_minimum_required(VERSION 3.16)
project(ApartmentsDiamond VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
#  Android: SDL2 собирается из исходников вместе с игрой.
#  Сборкой управляет Gradle-проект из каталога android/.
# ---------------------------------------------------------------------------
if(ANDROID)
    if(NOT KOLEK_SDL2_ROOT)
        # Если путь не передали, скачиваем SDL2 при первой конфигурации.
        # Для сборки без сети скачайте SDL2 вручную и передайте
        # -DKOLEK_SDL2_ROOT=/путь/к/SDL2-2.32.10
        include(FetchContent)
        set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
        set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "" FORCE)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(SDL2
            URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz
            URL_HASH SHA256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165)
        FetchContent_MakeAvailable(SDL2)
    endif()

    if(NOT TARGET SDL2::SDL2)
        set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
        set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "" FORCE)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        add_subdirectory("${KOLEK_SDL2_ROOT}" "${CMAKE_BINARY_DIR}/SDL2")
    endif()

    # SDLActivity ищет libmain.so и символ SDL_main.
    add_library(main SHARED src/main.cpp)
    target_include_directories(main PRIVATE src)
    target_compile_definitions(main PRIVATE KOLEK_ANDROID=1)
    target_link_libraries(main PRIVATE SDL2::SDL2)
    target_compile_options(main PRIVATE -Wall -Wextra)
    return()
endif()

# ---------------------------------------------------------------------------
#  Windows / Linux / macOS
# ---------------------------------------------------------------------------
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
