#include "gl_core_3_3.h"
#include <GLFW/glfw3.h>
#include "SDL2/SDL.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "main.hpp"
#include "nes.hpp"


uint8_t input;

#ifdef main
#undef main
#endif

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		std::cout << "nes rom.nes" << std::endl;
		exit(0);
	}
	const std::string infile = argv[1];

	// init audio
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		std::cout << "audio init failed\n";
		return 1;
	}

	SDL_AudioSpec desired = {44100, AUDIO_S8, 1, 0, 4096, 0, 0, 0};
	SDL_AudioDeviceID dev = SDL_OpenAudioDevice(0, 0, &desired, 0, SDL_AUDIO_ALLOW_ANY_CHANGE);
	if(dev == 0)
	{
		std::cout << "Failed to open audio: " << SDL_GetError() << "\n";
		return 1;
	}
	SDL_PauseAudioDevice(dev, 0);

	// init video
	if(!glfwInit())
	{
		exit(1);
	}

	// GLFWwindow* window = glfwCreateWindow(256, 240, "FRES++", NULL, NULL);
	GLFWwindow* window = glfwCreateWindow(512, 480, "FRES++", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(1);
	}

	glfwMakeContextCurrent(window);

	if(ogl_LoadFunctions() == ogl_LOAD_FAILED)
	{
		std::cout << "blopp" << std::endl;
		glfwTerminate();
		exit(1);
	}

	nes nes(infile);

	GLuint vao;
	initialize(vao, nes.ppu.GetPixelPtr());
	glfwSetKeyCallback(window, key_callback);

	uint32_t frameTime = 0;
	uint16_t frames = 0;

	while (!glfwWindowShouldClose(window))
	{
		std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

		nes.AdvanceFrame(input);

		glClear(GL_COLOR_BUFFER_BIT);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 240, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)nes.ppu.GetPixelPtr());
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
		glfwSwapBuffers(window);
		glfwPollEvents();

		//audio
		SDL_QueueAudio(dev, nes.apu.GetOutput(), nes.apu.sampleCount);
		nes.apu.ClearOutput();

		std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
		std::chrono::microseconds tus = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1);

		if(tus < std::chrono::microseconds(16639)) //16667 for 60hz, 16639 for 60.0988hz
		{
			std::this_thread::sleep_for(std::chrono::microseconds(16639) - tus);
		}

		std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
		frameTime += std::chrono::duration_cast<std::chrono::microseconds>(t3-t1).count();
		++frames;
		if(frameTime >= 1000000)
		{
			std::cout << frames << " fps" << std::endl;
			frameTime = 0;
			frames = 0;
		}
	}

	SDL_CloseAudioDevice(dev);
	glfwTerminate();
	return 0;
}


void initialize(GLuint &vao, const std::array<uint8_t, 256*240*3> *pixelPtr)
{
	// Use a Vertex Array Object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// 1 square (made by 2 triangles) to be rendered
	GLfloat vertices_position[8] = {-1.0f,1.0f,	1.0f,1.0f, 1.0f,-1.0f, -1.0f,-1.0f};
	GLfloat texture_coord[8] = {0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 0.0f,1.0f};
	uint8_t indices[6] = {0,1,2, 2,3,0};

	std::string vertex = "#version 150\n in vec4 position; in vec2 texture_coord; out vec2 texture_coord_from_vshader; void main() {gl_Position = position; texture_coord_from_vshader = texture_coord;}";
	std::string fragment = "#version 150\n in vec2 texture_coord_from_vshader; out vec4 out_color; uniform sampler2D texture_sampler; void main() {out_color = texture(texture_sampler, texture_coord_from_vshader);}";

	// Create a Vector Buffer Object that will store the vertices on video memory
	GLuint vbo;
	glGenBuffers(1, &vbo);

	// Allocate space for vertex positions and texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_position) + sizeof(texture_coord), NULL, GL_STATIC_DRAW);

	// Transfer the vertex positions:
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_position), vertices_position);

	// Transfer the texture coordinates:
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices_position), sizeof(texture_coord), texture_coord);

	// Create an Element Array Buffer that will store the indices array:
	GLuint eab;
	glGenBuffers(1, &eab);

	// Transfer the data from indices to eab
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eab);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Create a texture
	GLuint texture;
	glGenTextures(1, &texture);

	// Specify that we work with a 2D texture
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 240, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)pixelPtr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLuint shaderProgram = create_program(vertex, fragment);

	GLint position_attribute = glGetAttribLocation(shaderProgram, "position");

	// Specify how the data for position can be accessed
	glVertexAttribPointer(position_attribute, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Enable the attribute
	glEnableVertexAttribArray(position_attribute);

	// Texture coord attribute
	GLint texture_coord_attribute = glGetAttribLocation(shaderProgram, "texture_coord");
	glVertexAttribPointer(texture_coord_attribute, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)sizeof(vertices_position));
	glEnableVertexAttribArray(texture_coord_attribute);
}


GLuint create_program(std::string vertex, std::string fragment)
{
	// Load and compile the vertex and fragment shaders
	GLuint vertexShader = load_and_compile_shader(vertex, GL_VERTEX_SHADER);
	GLuint fragmentShader = load_and_compile_shader(fragment, GL_FRAGMENT_SHADER);

	// Attach the above shader to a program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	// Flag the shaders for deletion
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Link and use the program
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	return shaderProgram;
}


GLuint load_and_compile_shader(std::string &sName, GLenum shaderType)
{
	std::vector<char> buffer(sName.begin(), sName.end());
	buffer.push_back(0);
	const char *src = &buffer[0];

	// Compile the shader
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	// Check the result of the compilation
	GLint test;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &test);
	if(!test) {
		std::cerr << "Shader compilation failed with this message:" << std::endl;
		std::vector<char> compilation_log(512);
		glGetShaderInfoLog(shader, compilation_log.size(), NULL, &compilation_log[0]);
		std::cerr << &compilation_log[0] << std::endl;
		glfwTerminate();
		exit(-1);
	}
	return shader;
}


static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	input = 0;
	const std::vector<int> keys{GLFW_KEY_Z, GLFW_KEY_X, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT};
	uint8_t shift = 1;
	for(auto &v : keys)
	{
		if(glfwGetKey(window, v) == GLFW_PRESS)
		{
			input |= shift;
		}
		shift <<= 1;
	}


	if(action == GLFW_PRESS)
	{
		if(key == GLFW_KEY_ESCAPE)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

	return;
}
