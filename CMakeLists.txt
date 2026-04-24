cmake_minimum_required(VERSION 3.21)
project(pipe_bender_repo)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

file(GLOB_RECURSE SRC
    App/*.cpp
    Core/*.cpp
    Render/*.cpp
    Source/*.cpp
    Machine/*.cpp
)

add_executable(pipe_bender_repo ${SRC})

target_include_directories(pipe_bender_repo PRIVATE
    App Core Render Source Machine
)

target_link_libraries(pipe_bender_repo Qt6::Widgets Qt6::OpenGLWidgets)