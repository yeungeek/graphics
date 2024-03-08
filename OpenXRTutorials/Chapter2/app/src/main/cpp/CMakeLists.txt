cmake_minimum_required(VERSION 3.22.1)
set(PROJECT_NAME OpenXRSetup)

project("${PROJECT_NAME}")

message("CMake Project Name: ${PROJECT_NAME}")
message("CMake System: ${CMAKE_SYSTEM_NAME}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../cmake")

# XR_DOCS_TAG_END_CMakeModulePath
include("../../../../../cmake/graphics_api_select.cmake")

# XR_DOCS_TAG_BEGIN_FetchContent
include(FetchContent)

# openxr_loader - From github.com/KhronosGroup
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
# XR_DOCS_TAG_END_FetchContent

set(SOURCES
        main.cpp
        ../../../../../Common/GraphicsAPI.cpp
        ../../../../../Common/GraphicsAPI_D3D11.cpp
        ../../../../../Common/GraphicsAPI_D3D12.cpp
        ../../../../../Common/GraphicsAPI_OpenGL.cpp
        ../../../../../Common/GraphicsAPI_OpenGL_ES.cpp
        ../../../../../Common/GraphicsAPI_Vulkan.cpp
        ../../../../../Common/OpenXRDebugUtils.cpp
)

set(HEADERS
        ../../../../../Common/DebugOutput.h
        ../../../../../Common/GraphicsAPI.h
        ../../../../../Common/GraphicsAPI_D3D11.h
        ../../../../../Common/GraphicsAPI_D3D12.h
        ../../../../../Common/GraphicsAPI_OpenGL.h
        ../../../../../Common/GraphicsAPI_OpenGL_ES.h
        ../../../../../Common/GraphicsAPI_Vulkan.h
        ../../../../../Common/HelperFunctions.h
        ../../../../../Common/OpenXRDebugUtils.h
        ../../../../../Common/OpenXRHelper.h
)

#if (ANDROID)
message("CMake Android")
add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
        ../../../../../Common/
        # openxr
        "${openxr_SOURCE_DIR}/src/common"
        "${openxr_SOURCE_DIR}/external/include"
)

# export onCreate
set_property(TARGET ${PROJECT_NAME}
        APPEND_STRING
        PROPERTY LINK_FLAGS "-u ANativeActivity_onCreate"
)

# native_app_glue
include(AndroidNdkModules)
android_ndk_import_module_native_app_glue()

target_link_libraries(${PROJECT_NAME}
                        android
                        native_app_glue
                        openxr_loader
                        EGL
                        GLESv3
                        log)

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-cast-calling-convention)

AddGraphicsAPIDefine(${PROJECT_NAME})

# XR_DOCS_TAG_BEGIN_VulkanNDK
# VulkanNDK
find_library(vulkan-lib vulkan)
if (vulkan-lib)
    target_include_directories(${PROJECT_NAME} PUBLIC ${ANDROID_NDK}/sources/third_party/vulkan/src/include)
    target_link_libraries(${PROJECT_NAME} ${vulkan-lib})
    target_compile_definitions(${PROJECT_NAME} PUBLIC XR_TUTORIAL_USE_VULKAN)
endif ()
# XR_DOCS_TAG_END_VulkanNDK

# XR_DOCS_TAG_BEGIN_Gfxwrapper
include(../../../../../cmake/gfxwrapper.cmake)
if (TARGET openxr-gfxwrapper)
    target_link_libraries(${PROJECT_NAME} openxr-gfxwrapper)
    target_compile_definitions(${PROJECT_NAME} PUBLIC XR_TUTORIAL_USE_OPENGL_ES)

endif ()
# XR_DOCS_TAG_END_Gfxwrapper
#endif ()

# Add native_app_glue
#add_library(native_app_glue STATIC
#        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

#add_library(${CMAKE_PROJECT_NAME} SHARED
#        # List C/C++ source files with relative paths to this CMakeLists.txt.
#        main.cpp)

#target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
#        ${ANDROID_NDK}/sources/android/native_app_glue)

#target_link_libraries(${CMAKE_PROJECT_NAME}
#        # List libraries link to the target library
#        android
#        native_app_glue
#        EGL
#        GLESv3
#        log)