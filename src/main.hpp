#pragma once


GLuint load_and_compile_shader(std::string sname, GLenum shaderType);
GLuint create_program(std::string vertex, std::string fragment);
void initialize(GLuint &vao, const std::array<uint8_t, 256*240*3> *pixelPtr);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);