cmake_minimum_required(VERSION 3.25)
project(TerrainGenerationEditor)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(vendor/glad/glad)
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/imgui)

add_subdirectory(libs/Engine)
add_subdirectory(libs/TerrainGenerator)

add_subdirectory(src/Editor)
