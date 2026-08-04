#include "renderer.h"
#include <emscripten/html5.h>
#include <GLES2/gl2.h>
#include <cstdio>

Renderer::Renderer(int width, int height)
    : width(width), height(height) {

    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = false;
    attrs.depth = true;
    attrs.stencil = false;
    attrs.antialias = true;
    attrs.majorVersion = 1; // WebGL1

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx =
        emscripten_webgl_create_context("#canvas", &attrs);

    emscripten_webgl_make_context_current(ctx);

    const char* vsSrc =
        "attribute vec2 position;\n"
        "void main() {\n"
        "    gl_Position = vec4(position, 0.0, 1.0);\n"
        "}\n";

    const char* fsSrc =
        "void main() {\n"
        "    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
        "}\n";

    triangleShader = new Shader(vsSrc, fsSrc);
}


void Renderer::clear(float r, float g, float b) {
    glViewport(0, 0, 800, 600);   // hardcoded for now
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::drawTriangle() {
    triangleShader->use();

    float vertices[] = {
        0.0f,  0.5f,
       -0.5f, -0.5f,
        0.5f, -0.5f
    };

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLint posLoc = triangleShader->getAttribLocation("position");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(posLoc);
    glDeleteBuffers(1, &vbo);
}


