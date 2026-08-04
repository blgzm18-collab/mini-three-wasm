#pragma once
#include <GLES2/gl2.h>
#include "shader.h"
#include "../math/transform.h"

class Mesh {
public:
    Mesh(float* vertices, int vertexCount, Shader* shader);
    ~Mesh();

    void draw();

    Transform transform;

private:
    GLuint vbo;
    int vertexCount;
    Shader* shader;
};
