cmake_minimum_required(VERSION 2.8.3)

set(project_name nes)
project(${project_name})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -msse2 -mfpmath=sse")
add_definitions(-lglfw3 -lglew32 -lopengl32)

set(source_files
	src/main.cpp
	)

set(header_files
	src/main.hpp
	)

add_executable(${project_name} ${header_files} ${source_files})
target_link_libraries(nes ${GLFW_LIBRARY} ${GLEW_LIBRARY} opengl32)
