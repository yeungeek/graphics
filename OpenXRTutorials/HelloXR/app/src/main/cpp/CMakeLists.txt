cmake_minimum_required(VERSION 3.22.1)
set(PROJECT_NAME HelloXR)

project("${PROJECT_NAME}")

message("CMake Project Name: ${PROJECT_NAME}")
message("CMake Binary: ${CMAKE_CURRENT_BINARY_DIR}")

if(ANDROID)
    add_definitions(-DXR_USE_PLATFORM_ANDROID)

    set(HELLOXR_DEFAULT_GRAPHICS_PLUGIN "OpenGLES" CACHE STRING "Which graphics plugin should be used by default for Hello XR artifacts?")
    # These are the recognized options: provide them as a helper in the CMake GUI
    set_property(CACHE HELLOXR_DEFAULT_GRAPHICS_PLUGIN PROPERTY STRINGS "OpenGLES" "Vulkan")

#    if(HELLOXR_DEFAULT_GRAPHICS_PLUGIN STREQUAL "OpenGLES")
#        message(STATUS "hello_xr will default to OpenGL ES graphics plugin")
#        target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_GRAPHICS_PLUGIN_OPENGLES)
#    elseif(HELLOXR_DEFAULT_GRAPHICS_PLUGIN STREQUAL "Vulkan")
#        message(STATUS "hello_xr will default to Vulkan graphics plugin")
#        target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_GRAPHICS_PLUGIN_VULKAN)
#    endif()
endif()

include(FetchContent)
## openxr_loader - From github.com/KhronosGroup
set(BUILD_TESTS
        OFF
        CACHE INTERNAL "Build Tests")

set(BUILD_API_LAYERS
        OFF
        CACHE INTERNAL "Use OpenXR Layers")

FetchContent_Declare(
        OpenXR
        URL_HASH MD5=81930f0ccecdca852906e1a22aee4a45
        URL https://github.com/KhronosGroup/OpenXR-SDK-Source/archive/refs/tags/release-1.0.28.zip
        SOURCE_DIR
        openxr
)

FetchContent_MakeAvailable(OpenXR)
## end openxr_loader

file(GLOB LOCAL_HEADERS "common/*.h")
file(GLOB LOCAL_SOURCE "common/*.cpp")

set(SOURCES
        main.cpp
        ${LOCAL_SOURCE}
        ${LOCAL_HEADERS}
)

add_library(${PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        ${SOURCES})

# native_app_glue
include(AndroidNdkModules)
android_ndk_import_module_native_app_glue()

target_link_libraries(${PROJECT_NAME} OpenXR::openxr_loader)
target_link_libraries(${PROJECT_NAME} ${ANDROID_LIBRARY} ${ANDROID_LOG_LIBRARY})

target_link_libraries(${PROJECT_NAME}
        native_app_glue
        openxr_loader
        EGL
        GLESv3)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        "openxr/src"
        "${openxr_SOURCE_DIR}/src/common"
        "${openxr_SOURCE_DIR}/external/include"
)