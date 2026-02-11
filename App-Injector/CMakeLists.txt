# App-Injector/CMakeLists.txt

file(GLOB_RECURSE APP_INJECTOR_SOURCES
    src/*.h
    src/*.cpp
)

if(WL_HEADLESS)
    set(APP_INJECTOR_TARGET App-Injector-Headless)
else()
    set(APP_INJECTOR_TARGET App-Injector)
endif()

if(APPLE AND NOT WL_HEADLESS)
    add_executable(${APP_INJECTOR_TARGET} MACOSX_BUNDLE ${APP_INJECTOR_SOURCES})
else()
    add_executable(${APP_INJECTOR_TARGET} ${APP_INJECTOR_SOURCES})
endif()

target_include_directories(${APP_INJECTOR_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/Walnut/vendor/glm
    ${CMAKE_SOURCE_DIR}/Walnut/vendor/spdlog/include
    ${CMAKE_SOURCE_DIR}/Walnut/vendor/yaml-cpp/include
    ${CMAKE_SOURCE_DIR}/Walnut/Walnut/Source
    ${CMAKE_SOURCE_DIR}/Walnut/Walnut-Modules/Walnut-Networking/Source
    ${CMAKE_SOURCE_DIR}/App-Common
    ${CMAKE_SOURCE_DIR}/App-Injector/src
)

if(WL_HEADLESS)
    target_include_directories(${APP_INJECTOR_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/Walnut/Walnut/Platform/Headless
    )
else()
    target_include_directories(${APP_INJECTOR_TARGET} PRIVATE
        ${CMAKE_SOURCE_DIR}/Walnut/vendor/imgui
        ${CMAKE_SOURCE_DIR}/Walnut/vendor/glfw/include
        ${CMAKE_SOURCE_DIR}/Walnut/Walnut/Platform/GUI
    )
endif()

target_compile_definitions(${APP_INJECTOR_TARGET} PRIVATE
    YAML_CPP_STATIC_DEFINE
)

if(WL_HEADLESS)
    target_compile_definitions(${APP_INJECTOR_TARGET} PRIVATE WL_HEADLESS)
    
    target_link_libraries(${APP_INJECTOR_TARGET} PRIVATE
        Walnut-Headless
        Walnut-Networking
        yaml-cpp
    )
else()
    target_link_libraries(${APP_INJECTOR_TARGET} PRIVATE
        Walnut
        Walnut-Networking
        ImGui
        GLFW
        yaml-cpp
        Vulkan::Vulkan
    )
endif()

if(APPLE)
    set_target_properties(${APP_INJECTOR_TARGET} PROPERTIES
        BUILD_RPATH "@executable_path"
        INSTALL_RPATH "@executable_path"
    )
    
    if(NOT WL_HEADLESS)
        target_link_libraries(${APP_INJECTOR_TARGET} PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreFoundation"
        )
    endif()

    if(NOT WL_HEADLESS)
        set(MOLTENVK_DYLIB ${CMAKE_SOURCE_DIR}/Walnut/vendor/MoltenVK/libMoltenVK.dylib)
        add_custom_command(TARGET ${APP_INJECTOR_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MOLTENVK_DYLIB}
                $<TARGET_BUNDLE_DIR:${APP_INJECTOR_TARGET}>/Contents/MacOS/libvulkan.1.dylib
            COMMENT "Copying MoltenVK..."
        )

        set_source_files_properties(
                ${CMAKE_CURRENT_SOURCE_DIR}/res/Walnut-Icon.icns
                PROPERTIES MACOSX_PACKAGE_LOCATION Resources
        )
        target_sources(${APP_INJECTOR_TARGET} PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/res/Walnut-Icon.icns
        )
        set_target_properties(${APP_INJECTOR_TARGET} PROPERTIES
                MACOSX_BUNDLE_ICON_FILE "Walnut-Icon.icns"
        )
    endif()

    set(INJECTOR_ENTITLEMENTS ${CMAKE_SOURCE_DIR}/App-Injector/debug.entitlements)

    if(NOT WL_HEADLESS)
        add_custom_command(
                TARGET ${APP_INJECTOR_TARGET}
                POST_BUILD
                COMMAND codesign
                --force
                --sign -
                --entitlements ${INJECTOR_ENTITLEMENTS}
                --timestamp=none
                --deep
                $<TARGET_BUNDLE_DIR:${APP_INJECTOR_TARGET}>
                COMMENT "Codesigning ${APP_INJECTOR_TARGET} bundle with debug entitlements"
                VERBATIM
        )
    else()
        add_custom_command(
                TARGET ${APP_INJECTOR_TARGET}
                POST_BUILD
                COMMAND codesign
                --force
                --sign -
                --entitlements ${INJECTOR_ENTITLEMENTS}
                --timestamp=none
                $<TARGET_FILE:${APP_INJECTOR_TARGET}>
                COMMENT "Codesigning ${APP_INJECTOR_TARGET} executable with debug entitlements"
                VERBATIM
        )
    endif()

elseif(WIN32)
    target_link_libraries(${APP_INJECTOR_TARGET} PRIVATE Ws2_32)
    
    add_custom_command(TARGET ${APP_INJECTOR_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GNS_BIN_DIR}/libcrypto-3-x64.dll
            $<TARGET_FILE_DIR:${APP_INJECTOR_TARGET}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GNS_BIN_DIR}/libprotobufd.dll
            $<TARGET_FILE_DIR:${APP_INJECTOR_TARGET}>
        COMMENT "Copying runtime DLLs..."
    )
endif()

if(APPLE AND NOT WL_HEADLESS)
    set_target_properties(${APP_INJECTOR_TARGET} PROPERTIES
            BUNDLE_OUTPUT_DIRECTORY
            ${CMAKE_SOURCE_DIR}/build/bin/${OUTPUT_DIR}
    )
else()
    set_target_properties(${APP_INJECTOR_TARGET} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/bin/${OUTPUT_DIR}/${APP_INJECTOR_TARGET}
    )
endif()
