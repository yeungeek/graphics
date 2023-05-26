cmake_minimum_required (VERSION 2.8.12)

project(Transformation)

find_package(Eigen3 REQUIRED CONFIG)
message("[DEBUG] EIGEN3_INCLUDE_DIRS: ${EIGEN3_INCLUDE_DIRS}")

include_directories("${EIGEN3_INCLUDE_DIR}")

add_executable(Transformation main.cpp)
