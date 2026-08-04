#pragma once
#include "shader.h"

class Renderer {
public:
    Renderer(int width, int height);
    void clear(float r, float g, float b);
    void drawTriangle();

private:
    int width;
    int height;
    Shader* triangleShader;
};
