cmake_minimum_required (VERSION 3.0)
cmake_policy(VERSION 3.0)

project(LearnOpenGL)

# 使用 C++ 17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Header
set(GLAD_H ${CMAKE_SOURCE_DIR}/includes)
set(GLFW_H /usr/local/include)
include_directories(${GLAD_H} ${GLFW_H})


# 添加目标链接
set(GLFW_LINK /usr/local/lib/libglfw.3.dylib)
link_libraries(${GLFW_LINK})

# for
set(CHAPTERS
    learnopengl
    thecherno
)

set(learnopengl
    1.getting_started/1.1.hello_window
    1.getting_started/1.2.hello_triangle
)

set(thecherno
    example
)

add_library(GLAD "src/glad.c")
set(LIBS ${LIBS} GLAD)


function(create_project_from_sources chapter demo)
    file(GLOB SOURCE
            "src/${chapter}/${demo}/*.h"
            "src/${chapter}/${demo}/*.cpp"
            "src/${chapter}/${demo}/*.vs"
            "src/${chapter}/${demo}/*.fs"
            "src/${chapter}/${demo}/*.gs"
    )
    
    # string(FIND ${demo} "_" index)
    # if (index STRGREATER -1)
    #     SET(replaced "")
    #     string(REPLACE "/" "_" replaced ${demo})
    #     set(NAME "${chapter}__${replaced}")
    #     message("name: " ${NAME})
    # else()
    #     set(NAME "${chapter}__${demo}")
    #     message("_name: " ${NAME})
    # endif()

    # if (demo STREQUAL "")
    #     SET(replaced "")
    #     string(REPLACE "/" "_" replaced ${chapter})
    #     set(NAME ${replaced})
    # else()
    #     set(NAME "${chapter}__${demo}")
    # endif()

    foreach(v ${SOURCE})
        # file relative path from src/ 
        string(REGEX MATCH "src/.*" relative_path ${v})
        message("relative_path: ${relative_path}")
        # delete string "src/"
        string(REGEX REPLACE "src/" "" target_name ${relative_path})
        # message("target_name: ${target_name}")
        # rename '/' and '.' to '_'
        string(REGEX REPLACE "/|\\." "_" target_name ${target_name})
        string(REGEX REPLACE "_cpp" "" target_name ${target_name})
        message("target_name: ${target_name}")
        add_executable(${target_name} ${v})
        target_link_libraries(${target_name} ${LIBS})
    endforeach(v)

    # add_executable(${NAME} ${SOURCE})
    # target_link_libraries(${NAME} ${LIBS})

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