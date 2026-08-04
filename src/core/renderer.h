#pragma once
#include <GLES2/gl2.h>
#include "shader.h"
#include "../math/transform.h"
#include "../math/mat4.h"
#include "../math/vec3.h"
#include "mesh.h"


class Renderer {
public:
    Renderer(int width, int height);
    void clear(float r, float g, float b);
    void drawTriangle();

private:
    int width;
    int height;

    Shader* triangleShader;
    Mesh* triangleMesh;
};
