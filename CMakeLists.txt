cmake_minimum_required(VERSION 2.8.3)

set(project_name nes)
project(${project_name})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_path(GLFW_INCLUDE GLFW/glfw3.h PATHS NO_DEFAULT_PATH DOC "GLFW include path")
find_path(GLFW_LIB glfw3.dll PATHS NO_DEFAULT_PATH DOC "GLFW lib path")
include_directories(${GLFW_INCLUDE})
include_directories(${GLFW_LIB})
find_library(GLFW_LIBRARY "glfw3" HINTS ${GLFW_LIB})


find_path(SDL_INCLUDE SDL2/SDL.h PATHS NO_DEFAULT_PATH DOC "SDL include path")
find_path(SDL_LIB libSDL2.a PATHS NO_DEFAULT_PATH DOC "SDL lib path")
include_directories(${SDL_INCLUDE})
include_directories(${SDL_LIB})
find_library(SDL_LIBRARY "libSDL2.dll.a")
find_library(SDL_LIBRARY2 "libSDL2main.a")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -march=native") #-flto
add_definitions(-lglfw3 -lopengl32 -lSDL2main -lSDL2)

set(source_files
	src/main.cpp
	src/nes.cpp
	src/apu.cpp
	src/ppu.cpp
	src/gl_core_3_3.c
	)

set(header_files
	src/main.hpp
	src/nes.hpp
	src/apu.hpp
	src/ppu.hpp
	src/gl_core_3_3.h
	)

add_executable(${project_name} ${header_files} ${source_files})
target_link_libraries(nes ${GLFW_LIBRARY} opengl32 ${SDL_LIBRARY} ${SDL_LIBRARY2})
