#pragma once


void initialize(GLuint &vao, const uint8_t *const pixelPtr);
GLuint create_program(std::string vertex, std::string fragment);
GLuint load_and_compile_shader(std::string &sName, GLenum shaderType);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
