cmake_minimum_required(VERSION 3.25)
project(TerrainGenerator)

set(CMAKE_CXX_STANDARD 17)

set(__src
        TerrainGenerator.h TerrainGenerator.cpp
        Terrain.h Terrain.cpp)

add_library(${PROJECT_NAME} ${__src})

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        glad
        opengl32
        )

target_include_directories(${PROJECT_NAME} PRIVATE
        ../../vendor/glad/glad/include
        ../../vendor/glfw/include
        ../../vendor/glm
        ../../vendor/imgui
        ../../vendor/stb
        ../
        )
