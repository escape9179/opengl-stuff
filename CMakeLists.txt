cmake_minimum_required(VERSION 3.26)
project(TetrisGL)

set(CMAKE_CXX_STANDARD 17)

add_executable(TetrisGL main.cpp lodepng.cpp)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
target_link_libraries(TetrisGL GLEW::glew ${OPENGL_LIBRARIES})
