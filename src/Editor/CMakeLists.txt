cmake_minimum_required(VERSION 3.25)
project(TerrainEditor)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} main.cpp TerrainEditor.cpp TerrainEditor.h)

target_link_libraries(${PROJECT_NAME} PUBLIC
        Engine
)

target_include_directories(${PROJECT_NAME} PUBLIC
        ../../vendor/glad/glad/glad/include
        ../../vendor/glfw/include
        ../../vendor/glm/
        ../../vendor/imgui/
        ../../vendor/stb/stb
        ../../libs
)