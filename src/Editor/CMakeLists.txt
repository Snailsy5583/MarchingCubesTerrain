cmake_minimum_required(VERSION 3.25)
project(TerrainEditor)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME}
        main.cpp TerrainEditor.cpp TerrainEditor.h
        FlyCamera.cpp
        FlyCamera.h
        ScalarFieldEditor.cpp
        ScalarFieldEditor.h)

target_link_libraries(${PROJECT_NAME} PUBLIC
        Engine
        TerrainGenerator
        ImGui
        tinyexpr
)

target_include_directories(${PROJECT_NAME} PUBLIC
        ../../vendor/glad/glad/include
        ../../vendor/glfw/include
        ../../vendor/glm/
        ../../vendor/imgui/
        ../../vendor/stb/stb
        ../../vendor/tinyexpr
        ../../libs
)

add_custom_target(copy_shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders"
)
add_custom_target(copy_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_CURRENT_BINARY_DIR}/assets"
)
add_dependencies(TerrainEditor copy_shaders copy_assets)