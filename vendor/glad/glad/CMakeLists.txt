cmake_minimum_required(VERSION 3.25)
project(glad)

set(CMAKE_CXX_STANDARD 17)

############################### source files ###################################
set(__src
    src/glad.c
)

set(__include
    include/glad/glad.h
    include/KHR/khrplatform.h
)


add_library(${PROJECT_NAME} ${__src} ${__include})

target_include_directories(${PROJECT_NAME} PUBLIC
        include
)