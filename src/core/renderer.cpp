#include "renderer.h"
#include <emscripten/html5.h>
#include <emscripten/emscripten.h>
#include <GLES2/gl2.h>
#include <cstdio>
#include "../math/mat4.h"
#include "../math/mat3.h"

// Global pointer for main loop
static Renderer* g_renderer = nullptr;

// Forward declare main_loop BEFORE using it
extern "C" void main_loop();

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
    attrs.majorVersion = 1;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx =
        emscripten_webgl_create_context("#canvas", &attrs);

    if (ctx <= 0) {
        printf("Failed to create WebGL context\n");
    } else {
        printf("WebGL context created successfully\n");
        emscripten_webgl_make_context_current(ctx);
    }

    // Shader
const char* vsSrc =
    "attribute vec3 position;\n"
    "attribute vec3 normal;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "uniform mat3 normalMatrix;\n"
    "varying vec3 vNormal;\n"
    "void main() {\n"
    "    vNormal = normalMatrix * normal;\n"
    "    gl_Position = projection * view * model * vec4(position, 1.0);\n"
    "}\n";

const char* fsSrc =
    "precision mediump float;\n"
    "varying vec3 vNormal;\n"
    "uniform vec3 lightDir;\n"
    "void main() {\n"
    "    float diffuse = max(dot(normalize(vNormal), normalize(lightDir)), 0.0);\n"
    "    gl_FragColor = vec4(vec3(diffuse), 1.0);\n"
    "}\n";


    triangleShader = new Shader(vsSrc, fsSrc);

    // Camera matrices
    triangleShader->use();

    Mat4 view = camera.getViewMatrix();
    Mat4 projection = camera.getProjectionMatrix((float)width / (float)height);

    GLint viewLoc = triangleShader->getUniformLocation("view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view.data);

    GLint projLoc = triangleShader->getUniformLocation("projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection.data);

    // Triangle vertices
    float vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };

    // 36 vertices (12 triangles)
    static float cubeVertices[] = {
        // Front face (Z+)
        -0.5f,-0.5f, 0.5f,   0,0,1,
        0.5f,-0.5f, 0.5f,   0,0,1,
        0.5f, 0.5f, 0.5f,   0,0,1,

        -0.5f,-0.5f, 0.5f,   0,0,1,
        0.5f, 0.5f, 0.5f,   0,0,1,
        -0.5f, 0.5f, 0.5f,   0,0,1,

        // Back face (Z-)
        -0.5f,-0.5f,-0.5f,   0,0,-1,
        0.5f, 0.5f,-0.5f,   0,0,-1,
        0.5f,-0.5f,-0.5f,   0,0,-1,

        -0.5f,-0.5f,-0.5f,   0,0,-1,
        -0.5f, 0.5f,-0.5f,   0,0,-1,
        0.5f, 0.5f,-0.5f,   0,0,-1,

        // Left face (X-)
        -0.5f,-0.5f,-0.5f,  -1,0,0,
        -0.5f,-0.5f, 0.5f,  -1,0,0,
        -0.5f, 0.5f, 0.5f,  -1,0,0,

        -0.5f,-0.5f,-0.5f,  -1,0,0,
        -0.5f, 0.5f, 0.5f,  -1,0,0,
        -0.5f, 0.5f,-0.5f,  -1,0,0,

        // Right face (X+)
        0.5f,-0.5f,-0.5f,   1,0,0,
        0.5f, 0.5f, 0.5f,   1,0,0,
        0.5f,-0.5f, 0.5f,   1,0,0,

        0.5f,-0.5f,-0.5f,   1,0,0,
        0.5f, 0.5f,-0.5f,   1,0,0,
        0.5f, 0.5f, 0.5f,   1,0,0,

        // Top face (Y+)
        -0.5f, 0.5f,-0.5f,   0,1,0,
        -0.5f, 0.5f, 0.5f,   0,1,0,
        0.5f, 0.5f, 0.5f,   0,1,0,

        -0.5f, 0.5f,-0.5f,   0,1,0,
        0.5f, 0.5f, 0.5f,   0,1,0,
        0.5f, 0.5f,-0.5f,   0,1,0,

        // Bottom face (Y-)
        -0.5f,-0.5f,-0.5f,   0,-1,0,
        0.5f,-0.5f, 0.5f,   0,-1,0,
        -0.5f,-0.5f, 0.5f,   0,-1,0,

        -0.5f,-0.5f,-0.5f,   0,-1,0,
        0.5f,-0.5f,-0.5f,   0,-1,0,
        0.5f,-0.5f, 0.5f,   0,-1,0
    };

    triangleMesh = new Mesh(vertices, 9, triangleShader);
    cubeMesh = new Mesh(cubeVertices, 36 * 6, triangleShader);
    cubeMesh->transform.position = Vec3(1.5f, 0.0f, 0.0f);



    // Initial transform
    triangleMesh->transform.position = Vec3(0.0f, 0.0f, 0.0f);

    // Register renderer for main loop
    g_renderer = this;

    // Start animation loop
    emscripten_set_main_loop(main_loop, 0, true);
}

void Renderer::clear(float r, float g, float b) {
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawTriangle() {
    triangleShader->use();

    // Triangle normal matrix
    Mat4 modelTri = triangleMesh->transform.getModelMatrix();
    Mat3 normalTri = modelTri.toMat3();
    GLint nLoc = triangleShader->getUniformLocation("normalMatrix");
    glUniformMatrix3fv(nLoc, 1, GL_FALSE, normalTri.data);

    GLint lightLoc = triangleShader->getUniformLocation("lightDir");
    glUniform3f(lightLoc, -0.5f, 1.0f, 0.3f);

    triangleMesh->transform.rotation.y += 0.01f;
    triangleMesh->draw();

    // Cube normal matrix
    Mat4 modelCube = cubeMesh->transform.getModelMatrix();
    Mat3 normalCube = modelCube.toMat3();
    glUniformMatrix3fv(nLoc, 1, GL_FALSE, normalCube.data);

    cubeMesh->transform.rotation.x += 0.01f;
    cubeMesh->transform.rotation.y += 0.02f;
    cubeMesh->draw();
}

// Main loop called every frame
extern "C" void main_loop() {
    g_renderer->clear(0.1f, 0.1f, 0.2f);
    g_renderer->drawTriangle();
}
