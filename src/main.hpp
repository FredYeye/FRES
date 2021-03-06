#pragma once

#include "nes.hpp"


void Initialize(const uint32_t *const pixelPtr);
GLuint CreateProgram();
GLuint LoadAndCompileShader(const std::string &shaderName, GLenum shaderType);
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void Scale3x(const uint32_t *const pixelPtr);

#ifdef ENABLE_IMGUI
void ImguiStuff(const Nes &nes); //ImGuiIO &io
#endif

uint8_t input = 0, input2 = 0;

std::array<uint32_t, 256*3 * 240*3> scaledOutput;
const int texWidth = 256*3;
const int texHeight = 240*3;

bool pauseEmu = false, frameAdvance = false;
