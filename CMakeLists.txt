cmake_minimum_required(VERSION 3.20)
project(RobloxExternal-macOS LANGUAGES C CXX OBJC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS_DIST "${CMAKE_CXX_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_DIST "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_EXE_LINKER_FLAGS_DIST "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin/${CMAKE_BUILD_TYPE}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin/${CMAKE_BUILD_TYPE}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin/${CMAKE_BUILD_TYPE}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})

if(APPLE)
    if(CMAKE_OSX_ARCHITECTURES)
        set(ARCH ${CMAKE_OSX_ARCHITECTURES})
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(ARCH "arm64")
    else()
        set(ARCH "x86_64")
    endif()
    set(OUTPUT_DIR "${CMAKE_BUILD_TYPE}-macosx-${ARCH}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin/${OUTPUT_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin/${OUTPUT_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin/${OUTPUT_DIR})
elseif(WIN32)
    set(OUTPUT_DIR "${CMAKE_BUILD_TYPE}-windows-x64")
elseif(UNIX)
    set(OUTPUT_DIR "${CMAKE_BUILD_TYPE}-linux-x64")
endif()

option(WL_HEADLESS "Build headless (no GUI)" OFF)
option(WL_BUILD_INJECTOR "Build App-Injector" ON)
option(WL_BUILD_ESPMANAGER "Build App-ESPManager dylib" ON)


if(NOT WL_HEADLESS)
    find_package(Vulkan REQUIRED)
    message(STATUS "Vulkan found: ${Vulkan_INCLUDE_DIRS}")
endif()

if(APPLE)
    add_compile_definitions(WL_PLATFORM_MACOS)
elseif(WIN32)
    add_compile_definitions(WL_PLATFORM_WINDOWS)
elseif(UNIX)
    add_compile_definitions(WL_PLATFORM_LINUX)
endif()

add_compile_definitions(
    $<$<CONFIG:Debug>:WL_DEBUG>
    $<$<CONFIG:Release>:WL_RELEASE>
    $<$<CONFIG:Dist>:WL_DIST>
)

if(WL_HEADLESS)
    add_compile_definitions(WL_HEADLESS)
endif()

add_subdirectory(Walnut)

if(WL_BUILD_INJECTOR)
    add_subdirectory(App-Injector)
endif()

if(WL_BUILD_ESPMANAGER AND APPLE)
    add_subdirectory(App-ESPManager)
endif()
