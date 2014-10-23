cmake_minimum_required(VERSION 2.6)

project(nes)

find_path(GLEW_INCLUDE GL/glew.h PATHS NO_DEFAULT_PATH DOC "GLEW include path")
find_path(GLEW_LIB glew32.dll PATHS NO_DEFAULT_PATH DOC "GLEW lib path")
include_directories(${GLEW_INCLUDE})
include_directories(${GLEW_LIB})
find_library(GLEW_LIBRARY "glew32" HINTS ${GLEW_LIB})

find_path(GLFW_INCLUDE GLFW/glfw3.h PATHS NO_DEFAULT_PATH DOC "GLFW include path")
find_path(GLFW_LIB glfw3.dll PATHS NO_DEFAULT_PATH DOC "GLFW lib path")
include_directories(${GLFW_INCLUDE})
include_directories(${GLFW_LIB})
find_library(GLFW_LIBRARY "glfw3" HINTS ${GLFW_LIB})

add_definitions(-std=c++11 -msse2 -mfpmath=sse -lglfw3 -lglew32 -lopengl32)

add_executable(nes main.cpp)
target_link_libraries(nes ${GLFW_LIBRARY} ${GLEW_LIBRARY} opengl32)