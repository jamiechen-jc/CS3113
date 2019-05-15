#ifndef Helper_h
#define Helper_h

#include "ShaderProgram.h"
#include <vector>

float lerp(float v0, float v1, float t);
void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing);

#endif