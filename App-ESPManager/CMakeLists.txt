# App-ESPManager/CMakeLists.txt

set(ESP_MANAGER_SOURCES
        src/esp_manager.cpp
        src/objc_shims.mm
)

add_library(App-ESPManager SHARED ${ESP_MANAGER_SOURCES})

set_target_properties(App-ESPManager PROPERTIES
        PREFIX "lib"
        SUFFIX ".dylib"
        OUTPUT_NAME "App-ESPManager"
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        OBJCXX_STANDARD 23
        OBJCXX_STANDARD_REQUIRED ON
)

target_include_directories(App-ESPManager PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/App-Common
)

if(APPLE)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(COREGRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
    find_library(WEBKIT_FRAMEWORK WebKit REQUIRED)
    find_library(UNIFORMTYPEIDENTIFIERS_FRAMEWORK UniformTypeIdentifiers REQUIRED)
    find_library(SCREENCAPTUREKIT_FRAMEWORK ScreenCaptureKit REQUIRED)
    find_library(COREMEDIA_FRAMEWORK CoreMedia REQUIRED)
    find_library(COREIMAGE_FRAMEWORK CoreImage REQUIRED)

    target_link_libraries(App-ESPManager PRIVATE
            ${FOUNDATION_FRAMEWORK}
            ${COCOA_FRAMEWORK}
            ${COREGRAPHICS_FRAMEWORK}
            ${WEBKIT_FRAMEWORK}
            ${UNIFORMTYPEIDENTIFIERS_FRAMEWORK}
            ${SCREENCAPTUREKIT_FRAMEWORK}
            ${COREMEDIA_FRAMEWORK}
            ${COREIMAGE_FRAMEWORK}
    )
endif()

target_compile_options(App-ESPManager PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
)

if(WL_HEADLESS)
    set_target_properties(App-ESPManager PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY
            ${CMAKE_SOURCE_DIR}/build/bin/${OUTPUT_DIR}/App-Injector-Headless
    )
else()
    set_target_properties(App-ESPManager PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY
            $<TARGET_BUNDLE_CONTENT_DIR:App-Injector>/MacOS
    )
endif()

if(APPLE)
    set_target_properties(App-ESPManager PROPERTIES
            BUILD_RPATH "@loader_path"
            INSTALL_RPATH "@loader_path"
    )
endif()
