cmake_minimum_required (VERSION 3.0)
cmake_policy(VERSION 3.0)

project(LearnOpenGL)

# 使用 C++ 17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
SET(EXECUTABLE_OUTPUT_PATH “${CMAKE_SOURCE_DIR}/bin/“)

if(WIN32)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif(WIN32)

# Header
set(GLAD_H ${CMAKE_SOURCE_DIR}/includes)
set(GLFW_H /usr/local/include)
include_directories(${GLAD_H} ${GLFW_H})
include_directories(${CMAKE_SOURCE_DIR}/includes)

# 添加目标链接
set(GLFW_LINK /usr/local/lib/libglfw.3.dylib)
link_libraries(${GLFW_LINK})

# for
set(CHAPTERS
    1.getting_started
)

set(1.getting_started
    1.1.hello_window
    2.1.hello_triangle
    2.2.hello_triangle_indexed
    2.3.hello_triangle_e1
    3.1.shaders_uniform
    3.2.shader_class
    4.1.textures
)

configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)

add_library(STB_IMAGE "src/stb_image.cpp")
set(LIBS ${LIBS} STB_IMAGE)

add_library(GLAD "src/glad.c")
set(LIBS ${LIBS} GLAD)

macro(makeLink src dest target)
  add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${src} ${dest}  DEPENDS  ${dest} COMMENT "mklink ${src} -> ${dest}")
endmacro()

# function(create_project_from_sources chapter demo)
#     file(GLOB SOURCE
#             "src/${chapter}/${demo}/*.h"
#             "src/${chapter}/${demo}/*.cpp"
#             # "src/${chapter}/${demo}/*.vs"
#             # "src/${chapter}/${demo}/*.fs"
#             "src/${chapter}/${demo}/*.gs"
#     )
    
#     # string(FIND ${demo} "_" index)
#     # if (index STRGREATER -1)
#     #     SET(replaced "")
#     #     string(REPLACE "/" "_" replaced ${demo})
#     #     set(NAME "${chapter}__${replaced}")
#     #     message("name: " ${NAME})
#     # else()
#     #     set(NAME "${chapter}__${demo}")
#     #     message("_name: " ${NAME})
#     # endif()

#     # if (demo STREQUAL "")
#     #     SET(replaced "")
#     #     string(REPLACE "/" "_" replaced ${chapter})
#     #     set(NAME ${replaced})
#     # else()
#     #     set(NAME "${chapter}__${demo}")
#     # endif()

#     foreach(v ${SOURCE})
#         # file relative path from src/ 
#         string(REGEX MATCH "src/.*" relative_path ${v})
#         message("relative_path: ${relative_path}")
#         # delete string "src/"
#         string(REGEX REPLACE "src/" "" target_name ${relative_path})
#         # message("target_name: ${target_name}")
#         # rename '/' and '.' to '_'
#         string(REGEX REPLACE "/|\\." "_" target_name ${target_name})
#         string(REGEX REPLACE "_cpp" "" target_name ${target_name})
#         message("target_name: ${target_name}")
#         add_executable(${target_name} ${v})
#         target_link_libraries(${target_name} ${LIBS})
#         set(NAME ${target_name})
#     endforeach(v)

#     if(MSVC)
#         target_compile_options(${NAME} PRIVATE /std:c++17 /MP)
#         target_link_options(${NAME} PUBLIC /ignore:4099)
#     endif(MSVC)
#     if(WIN32)
#         set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}")
#         set_target_properties(${NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}/Debug")
#     elseif(UNIX AND NOT APPLE)
#         set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}")
#     elseif(APPLE)
#         set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}")
#         set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/${chapter}")
#         set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/${chapter}")
#     endif(WIN32)

#     # copy shader files to build directory
#     file(GLOB SHADERS
#              "src/${chapter}/${demo}/*.vs"
#              "src/${chapter}/${demo}/*.fs"
#              "src/${chapter}/${demo}/*.gs"
#     )

#     foreach(SHADER ${SHADERS})
#         message("##### shaders ${SHADERS}")
#         if(WIN32)
#             # configure_file(${SHADER} "test")
#             add_custom_command(TARGET ${NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SHADER} $<TARGET_FILE_DIR:${NAME}>)
#         elseif(UNIX AND NOT APPLE)
#             file(COPY ${SHADER} DESTINATION ${CMAKE_SOURCE_DIR}/bin/${chapter})
#         elseif(APPLE)
#             # create symbolic link for *.vs *.fs *.gs
#             get_filename_component(SHADERNAME ${SHADER} NAME)
#             makeLink(${SHADER} ${CMAKE_SOURCE_DIR}/bin/${chapter}/${SHADERNAME} ${NAME})
#         endif(WIN32)
#     endforeach(SHADER)
# endfunction()

function(create_project_from_sources chapter demo)
	file(GLOB SOURCE
            "src/${chapter}/${demo}/*.h"
            "src/${chapter}/${demo}/*.cpp"
            "src/${chapter}/${demo}/*.vs"
            "src/${chapter}/${demo}/*.fs"
            "src/${chapter}/${demo}/*.gs"
    )
	if (demo STREQUAL "")
		SET(replaced "")
		string(REPLACE "/" "_" replaced ${chapter})
		set(NAME ${replaced})
	else()
		set(NAME "${chapter}__${demo}")
	endif()

    # message("name "${NAME} ${SOURCE})

    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} ${LIBS})
    if(MSVC)
		target_compile_options(${NAME} PRIVATE /std:c++17 /MP)
        target_link_options(${NAME} PUBLIC /ignore:4099)
    endif(MSVC)
    if(WIN32)
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}")
        set_target_properties(${NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}/Debug")
    elseif(UNIX AND NOT APPLE)
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}")
    elseif(APPLE)
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${chapter}")
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/${chapter}")
        set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/${chapter}")
    endif(WIN32)
    # copy shader files to build directory
    file(GLOB SHADERS
             "src/${chapter}/${demo}/*.vs"
             # "src/${chapter}/${demo}/*.frag"
             "src/${chapter}/${demo}/*.fs"
             "src/${chapter}/${demo}/*.gs"
    )
    foreach(SHADER ${SHADERS})
        if(WIN32)
            # configure_file(${SHADER} "test")
            add_custom_command(TARGET ${NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SHADER} $<TARGET_FILE_DIR:${NAME}>)
        elseif(UNIX AND NOT APPLE)
            file(COPY ${SHADER} DESTINATION ${CMAKE_SOURCE_DIR}/bin/${chapter})
        elseif(APPLE)
            # create symbolic link for *.vs *.fs *.gs
            get_filename_component(SHADERNAME ${SHADER} NAME)
            message("SHADERNAME ${SHADERNAME} ${SHADER} ${NAME}")
            makeLink(${SHADER} ${CMAKE_SOURCE_DIR}/bin/${chapter}/${SHADERNAME} ${NAME})
        endif(WIN32)
    endforeach(SHADER)
endfunction()

foreach(CHAPTER ${CHAPTERS})
    foreach(DEMO ${${CHAPTER}})
		create_project_from_sources(${CHAPTER} ${DEMO})
    endforeach(DEMO)
endforeach(CHAPTER)

# set(SOURCES src/glad.c src/1.getting_started/1.1_hello_window/hello_window.cpp)
# add_executable(LearnOpenGL ${SOURCES})

# if (APPLE)
#     target_link_libraries(LearnOpenGL "-framework OpenGL")
# endif()

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)