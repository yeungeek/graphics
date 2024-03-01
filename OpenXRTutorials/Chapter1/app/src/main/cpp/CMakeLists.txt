cmake_minimum_required(VERSION 3.22.1)
set(PROJECT_NAME OpenXRIntro)

project("${PROJECT_NAME}")

message("CMake Project Name: ${PROJECT_NAME}")

# Add native_app_glue
add_library(native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        main.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        android
        native_app_glue
        EGL
        android
        log)