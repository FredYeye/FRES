cmake_minimum_required(VERSION 2.8.3)
set(project_name nes)
project(${project_name})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -march=native") #-flto -static -fno-math-errno?

if(WIN32)
	message("bad choice")

	find_path(GLFW_INCLUDE GLFW/glfw3.h PATHS NO_DEFAULT_PATH DOC "GLFW include path")
	find_path(GLFW_LIB glfw3.dll PATHS NO_DEFAULT_PATH DOC "GLFW lib path")
	include_directories(${GLFW_INCLUDE})
	include_directories(${GLFW_LIB})
	find_library(GLFW_LIBRARY "glfw3" HINTS ${GLFW_LIB})

	add_definitions(-DWINDOWS -lglfw3 -lopengl32)

	set(source_files
		src/main.cpp
		src/nes.cpp
		src/apu.cpp
		src/ppu.cpp
		src/wasapi.cpp
		src/file.cpp
		src/gl_core_3_3.c
		)

	set(header_files
		src/main.hpp
		src/nes.hpp
		src/apu.hpp
		src/ppu.hpp
		src/wasapi.hpp
		src/file.hpp
		src/gl_core_3_3.h
		)

	add_executable(${project_name} ${header_files} ${source_files})
	target_link_libraries(nes ${GLFW_LIBRARY} opengl32 ole32 ksuser)
endif(WIN32)

if(UNIX)
	message("lucky! cha cha cha!")

	find_package(PkgConfig REQUIRED)
	pkg_search_module(GLFW REQUIRED glfw3)
	include_directories(${GLFW_INCLUDE_DIRS})

	find_package(OpenGL)

	add_definitions(-DLINUX -lglfw3)

	set(source_files
		src/main.cpp
		src/nes.cpp
		src/apu.cpp
		src/ppu.cpp
		src/alsa.cpp
		src/file.cpp
		src/gl_core_3_3.c
		)

	set(header_files
		src/main.hpp
		src/nes.hpp
		src/apu.hpp
		src/ppu.hpp
		src/alsa.hpp
		src/file.hpp
		src/gl_core_3_3.h
		)

	add_executable(${project_name} ${header_files} ${source_files})
	target_link_libraries(nes ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES} lasound)
endif(UNIX)
