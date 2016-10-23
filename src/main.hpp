#pragma once


void Initialize(GLuint &vao, const uint32_t *const pixelPtr);
GLuint CreateProgram();
GLuint LoadAndCompileShader(const std::string &shaderName, GLenum shaderType);
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

uint8_t input, input2;