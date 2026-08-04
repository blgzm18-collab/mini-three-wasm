#include "renderer.h"
#include <emscripten/html5.h>
#include <GLES2/gl2.h>
#include <cstdio>
#include "../math/mat4.h"
#include "../math/vec3.h"

Renderer::Renderer(int width, int height)
    : width(width), height(height)
{
    // Create WebGL context
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = false;
    attrs.depth = true;
    attrs.stencil = false;
    attrs.antialias = true;
    attrs.majorVersion = 1; // WebGL1

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx =
        emscripten_webgl_create_context("#canvas", &attrs);

    if (ctx <= 0) {
        printf("Failed to create WebGL context\n");
    } else {
        printf("WebGL context created successfully\n");
        emscripten_webgl_make_context_current(ctx);
    }

    // 3D shader
    const char* vsSrc =
        "attribute vec3 position;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model * vec4(position, 1.0);\n"
        "}\n";

    const char* fsSrc =
        "void main() {\n"
        "    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
        "}\n";

    triangleShader = new Shader(vsSrc, fsSrc);

    // Basic view and projection (identity for now)
    Mat4 view = Mat4::identity();
    Mat4 projection = Mat4::identity();

    triangleShader->use();

    GLint viewLoc = triangleShader->getUniformLocation("view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view.data);

    GLint projLoc = triangleShader->getUniformLocation("projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection.data);

    // Triangle vertices (3D)
    float vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };

    triangleMesh = new Mesh(vertices, 9, triangleShader);

    // Default transform: put it slightly in front
    triangleMesh->transform.position = Vec3(0.0f, 0.0f, 0.0f);
}

void Renderer::clear(float r, float g, float b) {
    glViewport(0, 0, width, height);
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::drawTriangle() {
    triangleMesh->draw();
}
