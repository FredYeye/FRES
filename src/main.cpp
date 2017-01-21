#include "gl_core_3_3.h"
#include <GLFW/glfw3.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "main.hpp"
#include "nes.hpp"

#ifdef WINDOWS
	#include "wasapi.hpp"
#else
	#include "alsa.hpp"
#endif


int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		std::cout << "nes rom.nes" << std::endl;
		exit(0);
	}
	const std::string infile = argv[1];

	// init video
	if(!glfwInit())
	{
		exit(1);
	}

	// GLFWwindow* window = glfwCreateWindow(256, 240, "FRES++", 0, 0);
	GLFWwindow* window = glfwCreateWindow(512, 480, "FRES++", 0, 0);
	if(!window)
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

	Nes nes(infile);

	GLuint vao;
	Initialize(vao, nes.ppu.GetPixelPtr());
	glfwSetKeyCallback(window, KeyCallback);

	// init audio
	Audio audio(nes.apu.GetOutput(), false);
	audio.StartAudio();

	// uint32_t frameTime = 0;
	// uint16_t frames = 0;

	while(!glfwWindowShouldClose(window))
	{
		// std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

		nes.AdvanceFrame(input, input2);

		glClear(GL_COLOR_BUFFER_BIT);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 240, GL_RGBA, GL_UNSIGNED_BYTE, nes.ppu.GetPixelPtr());
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

		audio.StreamSource(); // framerate controlled by audio playback
		nes.apu.sampleCount = 0;

		glfwSwapBuffers(window);
		glfwPollEvents();

		// std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
		// frameTime += std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
		// ++frames;
		// if(frameTime >= 1000000)
		// {
		// 	std::cout << frames << " fps" << std::endl;
		// 	frameTime = 0;
		// 	frames = 0;
		// }
	}

	audio.StopAudio();
	glfwTerminate();

	return 0;
}


void Initialize(GLuint &vao, const uint32_t *const pixelPtr)
{
	glGenVertexArrays(1, &vao); //Use a Vertex Array Object
	glBindVertexArray(vao);

	const int8_t verticesPosition[8] = {-1,1, 1,1, 1,-1, -1,-1}; //1 square (made by 2 triangles) to be rendered
	const uint8_t textureCoord[8] = {0,0, 1,0, 1,1, 0,1};
	const uint8_t indices[6] = {0,1,2, 2,3,0};

	GLuint vbo; //Create a Vector Buffer Object that will store the vertices on video memory
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo); //Allocate space for vertex positions and texture coordinates
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesPosition) + sizeof(textureCoord), 0, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verticesPosition), verticesPosition); //Transfer the vertex positions
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(verticesPosition), sizeof(textureCoord), textureCoord); //Transfer the texture coordinates

	GLuint eab; //Create an Element Array Buffer that will store the indices array
	glGenBuffers(1, &eab);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eab); //Transfer the data from indices to eab
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	GLuint texture; //Create a texture
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture); //Specify that we work with a 2D texture

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 240, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelPtr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	const GLuint shaderProgram = CreateProgram();

	GLint position_attribute = glGetAttribLocation(shaderProgram, "position");
	glVertexAttribPointer(position_attribute, 2, GL_BYTE, GL_FALSE, 0, 0); //Specify how the data for position can be accessed
	glEnableVertexAttribArray(position_attribute); //Enable the attribute

	GLint textureCoord_attribute = glGetAttribLocation(shaderProgram, "textureCoord"); //Texture coord attribute
	glVertexAttribPointer(textureCoord_attribute, 2, GL_UNSIGNED_BYTE, GL_FALSE, 0, (GLvoid*)sizeof(verticesPosition));
	glEnableVertexAttribArray(textureCoord_attribute);
}


GLuint CreateProgram()
{
	const std::string vertex =
	"#version 130\n"
	"in vec4 position; in vec2 textureCoord;"
	"out vec2 textureCoord_from_vshader;"
	"void main() {"
	"gl_Position = position;"
	"textureCoord_from_vshader = textureCoord;"
	"}";

	const std::string fragment =
	"#version 130\n"
	"in vec2 textureCoord_from_vshader;"
	"out vec4 out_color;"
	"uniform sampler2D texture_sampler;"
	"void main() {"
	"out_color = texture(texture_sampler, textureCoord_from_vshader);"
	"}";

	const GLuint vertexShader = LoadAndCompileShader(vertex, GL_VERTEX_SHADER); //Load and compile the vertex and fragment shaders
	const GLuint fragmentShader = LoadAndCompileShader(fragment, GL_FRAGMENT_SHADER);

	GLuint shaderProgram = glCreateProgram(); //Attach the above shader to a program
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	glDeleteShader(vertexShader); //Flag the shaders for deletion
	glDeleteShader(fragmentShader);

	glLinkProgram(shaderProgram); //Link and use the program
	glUseProgram(shaderProgram);

	return shaderProgram;
}


GLuint LoadAndCompileShader(const std::string &shaderName, GLenum shaderType)
{
	std::vector<char> buffer(shaderName.begin(), shaderName.end());
	buffer.push_back(0);
	const char *src = buffer.data();

	GLuint shader = glCreateShader(shaderType); //Compile the shader
	glShaderSource(shader, 1, &src, 0);
	glCompileShader(shader);

	GLint test; //Check the result of the compilation
	glGetShaderiv(shader, GL_COMPILE_STATUS, &test);
	if(test != GL_TRUE)
	{
		std::array<char, 512> compilation_log;
		glGetShaderInfoLog(shader, compilation_log.size(), 0, compilation_log.data());
		std::cout << "Shader compilation failed:" << std::endl << &compilation_log[0] << std::endl;
		glfwTerminate();
		exit(-1);
	}

	return shader;
}


static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	std::map<int, uint8_t> keys
	{
		{GLFW_KEY_Z,     0b00000001}, {GLFW_KEY_X,     0b00000010},
		{GLFW_KEY_S,     0b00000100}, {GLFW_KEY_A,     0b00001000},
		{GLFW_KEY_UP,    0b00010000}, {GLFW_KEY_DOWN,  0b00100000},
		{GLFW_KEY_LEFT,  0b01000000}, {GLFW_KEY_RIGHT, 0b10000000},
	};
	const uint8_t asd = (keys.find(key) != keys.end()) ? keys.at(key) : 0;
	if(action != GLFW_REPEAT) input ^= asd;

	std::map<int, uint8_t> keys2
	{
		{GLFW_KEY_G,  0b00000001}, {GLFW_KEY_H, 0b00000010},
		{GLFW_KEY_Y,  0b00000100}, {GLFW_KEY_T, 0b00001000},
		{GLFW_KEY_I,  0b00010000}, {GLFW_KEY_K, 0b00100000},
		{GLFW_KEY_J,  0b01000000}, {GLFW_KEY_L, 0b10000000},
	};
	const uint8_t asd2 = (keys2.find(key) != keys2.end()) ? keys2.at(key) : 0;
	if(action != GLFW_REPEAT) input2 ^= asd2;

	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}
