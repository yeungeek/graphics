cmake_minimum_required(VERSION 3.22.1)
set(PROJECT_NAME HelloXR)

project("${PROJECT_NAME}")

message("CMake Project Name: ${PROJECT_NAME}")
message("CMake Binary: ${CMAKE_CURRENT_BINARY_DIR}")
message("ANDROID ABI: ${ANDROID_ABI}")

if(ANDROID)
    add_definitions(-DXR_USE_PLATFORM_ANDROID -DXR_USE_GRAPHICS_API_OPENGL_ES)
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

#main.cpp
set(SOURCES
        sample.cpp
        ${LOCAL_SOURCE}
)

set(HEADERS
        ${LOCAL_HEADERS}
)

add_library(${PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        ${SOURCES} ${HEADERS})

# export onCreate
set_property(TARGET ${PROJECT_NAME}
        APPEND_STRING
        PROPERTY LINK_FLAGS "-u ANativeActivity_onCreate"
)

# native_app_glue
include(AndroidNdkModules)
android_ndk_import_module_native_app_glue()

if(TARGET openxr-gfxwrapper)
    target_link_libraries(${PROJECT_NAME} openxr-gfxwrapper)
endif()

# oepnxr_loader.so
add_library(rokid_loader SHARED IMPORTED)
set_target_properties(rokid_loader PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/openxr/lib/${ANDROID_ABI}/libopenxr_loader.so)

#official
#target_link_libraries(${PROJECT_NAME} OpenXR::openxr_loader)

target_link_libraries(${PROJECT_NAME} ${ANDROID_LIBRARY} ${ANDROID_LOG_LIBRARY})

target_link_libraries(${PROJECT_NAME}
        native_app_glue
        rokid_loader
        EGL
        GLESv3)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        "openxr/src/openxr"
        "${openxr_SOURCE_DIR}/src"
        "${openxr_SOURCE_DIR}/external/include"
)