// renderer.cpp
// Full implementation for the mini-three-wasm renderer (defensive GL guards)

#include "renderer.h"

#include <vector>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/val.h>
#include <emscripten/html5.h> 
#include <GLES3/gl3.h>
#else
// Native fallback headers (optional)
#include <GL/glew.h>
#endif

// -----------------------------
// Mesh tint accessors (emscripten)
// -----------------------------
#ifdef __EMSCRIPTEN__
emscripten::val Mesh::getTint() const {
    emscripten::val arr = emscripten::val::array();
    arr.set(0, tint[0]);
    arr.set(1, tint[1]);
    arr.set(2, tint[2]);
    arr.set(3, tint[3]);
    return arr;
}

void Mesh::setTint(const emscripten::val& arr) {
    // Defensive: check length and types
    if (!arr.isArray()) return;
    for (int i = 0; i < 4; ++i) {
        if (arr[i].isUndefined()) continue;
        tint[i] = arr[i].as<float>();
    }
}
#endif

// -----------------------------
// GL context guard
// -----------------------------
static inline bool hasGLContext() {
#ifdef __EMSCRIPTEN__
    // emscripten_webgl_get_current_context returns 0 when no context is current
    return emscripten_webgl_get_current_context() != 0;
#else
    // On native builds assume GL is available (caller is responsible)
    return true;
#endif
}

// -----------------------------
// Simple math helpers
// -----------------------------
static inline void identityMat4(float m[16]) {
    memset(m, 0, sizeof(float) * 16);
    m[0] = 1.0f; m[5] = 1.0f; m[10] = 1.0f; m[15] = 1.0f;
}

// -----------------------------
// Shader sources
// -----------------------------
static const char* vertexShaderSrc = R"(
#version 300 es
precision highp float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aColor;

uniform mat4 uProjView;
uniform mat4 uModel;

out vec4 vColor;
out vec3 vNormal;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    gl_Position = uProjView * worldPos;
    vNormal = mat3(uModel) * aNormal;
    vColor = aColor;
}
)";

static const char* fragmentShaderSrc = R"(
#version 300 es
precision highp float;

in vec4 vColor;
in vec3 vNormal;

uniform vec4 uTint;

out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float NdotL = max(dot(normalize(vNormal), lightDir), 0.0);
    vec3 ambient = vec3(0.12);
    vec3 diffuse = vec3(0.88) * NdotL;
    vec3 color = (ambient + diffuse) * vColor.rgb * uTint.rgb;
    outColor = vec4(color, vColor.a * uTint.a);
}
)";

// -----------------------------
// GL helper: compile/link
// -----------------------------
static GLuint compileShader(GLenum type, const char* src) {
    if (!hasGLContext()) {
        std::cerr << "[renderer] compileShader skipped: no GL context\n";
        return 0;
    }

    GLuint s = glCreateShader(type);
    if (!s) {
        std::cerr << "[renderer] glCreateShader returned 0\n";
        return 0;
    }
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len > 0 ? len : 1, '\0');
        glGetShaderInfoLog(s, len, nullptr, &log[0]);
        std::cerr << "Shader compile error: " << log << std::endl;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint linkProgram(GLuint vs, GLuint fs) {
    if (!hasGLContext()) {
        std::cerr << "[renderer] linkProgram skipped: no GL context\n";
        return 0;
    }

    GLuint p = glCreateProgram();
    if (!p) {
        std::cerr << "[renderer] glCreateProgram returned 0\n";
        return 0;
    }
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len > 0 ? len : 1, '\0');
        glGetProgramInfoLog(p, len, nullptr, &log[0]);
        std::cerr << "Program link error: " << log << std::endl;
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

// -----------------------------
// Renderer implementation
// -----------------------------
Renderer::Renderer() {
    // default ctor: no viewport stored
    identityMat4(projView);
}

Renderer::Renderer(int width, int height) {
    // Setup a simple orthographic projection by default
    float aspect = (height == 0) ? 1.0f : (float)width / (float)height;
    float viewWidth = 20.0f;
    float viewHeight = viewWidth / aspect;

    float left = -viewWidth * 0.5f;
    float right = viewWidth * 0.5f;
    float bottom = -viewHeight * 0.5f;
    float top = viewHeight * 0.5f;
    float nearp = -50.0f;
    float farp = 50.0f;

    float rl = 1.0f / (right - left);
    float tb = 1.0f / (top - bottom);
    float fn = 1.0f / (farp - nearp);

    memset(projView, 0, sizeof(projView));
    projView[0] = 2.0f * rl;
    projView[5] = 2.0f * tb;
    projView[10] = -2.0f * fn;
    projView[12] = -(right + left) * rl;
    projView[13] = -(top + bottom) * tb;
    projView[14] = -(farp + nearp) * fn;
    projView[15] = 1.0f;
}

Renderer::~Renderer() {
    for (auto m : sceneMeshes) {
        if (m) {
            if (m->uploaded) {
                if (m->vbo) glDeleteBuffers(1, &m->vbo);
                if (m->ibo) glDeleteBuffers(1, &m->ibo);
                if (m->vao) glDeleteVertexArrays(1, &m->vao);
            }
            delete m;
        }
    }
    sceneMeshes.clear();

    if (program) {
        glDeleteProgram(program);
        program = 0;
    }
}

bool Renderer::compileShaders() {
    if (!hasGLContext()) {
        std::cerr << "[renderer] compileShaders skipped: no GL context\n";
        return false;
    }

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    if (!vs) return false;
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    if (!fs) {
        glDeleteShader(vs);
        return false;
    }
    program = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!program) return false;

    uProjViewLoc = glGetUniformLocation(program, "uProjView");
    uModelLoc = glGetUniformLocation(program, "uModel");
    uColorLoc = glGetUniformLocation(program, "uTint");
    return true;
}

bool Renderer::init() {
    if (!hasGLContext()) {
        std::cerr << "[renderer] init skipped: no GL context\n";
        return false;
    }

    if (!compileShaders()) {
        std::cerr << "[renderer] init failed: compileShaders failed\n";
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // default projView identity if not set by ctor
    // already set in constructors

    return true;
}

void Renderer::uploadMesh(Mesh* mesh) {
    if (!mesh) return;
    if (!hasGLContext()) {
        std::cerr << "[renderer] uploadMesh skipped: no GL context\n";
        return;
    }
    if (mesh->uploaded) return;

    glGenVertexArrays(1, &mesh->vao);
    glBindVertexArray(mesh->vao);

    glGenBuffers(1, &mesh->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertices.size() * sizeof(Vertex), mesh->vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &mesh->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(uint32_t), mesh->indices.data(), GL_STATIC_DRAW);

    // Vertex attributes
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1); // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2); // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));

    glEnableVertexAttribArray(3); // color
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, color));

    glBindVertexArray(0);

    mesh->uploaded = true;
}

void Renderer::drawMesh(Mesh* mesh) {
    if (!mesh) return;
    if (!hasGLContext()) {
        std::cerr << "[renderer] drawMesh skipped: no GL context\n";
        return;
    }
    if (!mesh->uploaded) uploadMesh(mesh);

    if (!program) {
        std::cerr << "[renderer] drawMesh skipped: program not compiled\n";
        return;
    }

    glUseProgram(program);

    // model identity
    float model[16];
    identityMat4(model);

    glUniformMatrix4fv(uProjViewLoc, 1, GL_FALSE, projView);
    glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, model);

    // set tint uniform from mesh->tint
    float tintArr[4] = { mesh->tint[0], mesh->tint[1], mesh->tint[2], mesh->tint[3] };
    glUniform4fv(uColorLoc, 1, tintArr);

    glBindVertexArray(mesh->vao);

    GLenum mode = (mesh->type == LINES) ? GL_LINES : GL_TRIANGLES;
    glDrawElements(mode, (GLsizei)mesh->indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::renderFrame() {
    if (!hasGLContext()) {
        std::cerr << "[renderer] renderFrame skipped: no GL context\n";
        return;
    }

    // Clear with a neutral background
    glClearColor(0.12f, 0.14f, 0.16f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto m : sceneMeshes) {
        drawMesh(m);
    }
}

void Renderer::addMesh(Mesh* mesh) {
    if (!mesh) return;
    sceneMeshes.push_back(mesh);
}

void Renderer::clear(float r, float g, float b) {
    if (!hasGLContext()) {
        std::cerr << "[renderer] clear skipped: no GL context\n";
        return;
    }
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::drawTriangle() {
    if (!hasGLContext()) {
        std::cerr << "[renderer] drawTriangle skipped: no GL context\n";
        return;
    }

    // Simple immediate triangle for smoke tests
    static GLuint triVao = 0;
    static GLuint triVbo = 0;
    if (!triVao) {
        float verts[] = {
            0.0f,  0.5f, 0.0f,
           -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f
        };
        glGenVertexArrays(1, &triVao);
        glBindVertexArray(triVao);
        glGenBuffers(1, &triVbo);
        glBindBuffer(GL_ARRAY_BUFFER, triVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindVertexArray(0);
    }

    // Use simple shader if available
    if (program) glUseProgram(program);
    // set a default tint
    if (uColorLoc >= 0) {
        float tint[4] = {1.0f, 0.6f, 0.2f, 1.0f};
        glUniform4fv(uColorLoc, 1, tint);
    }

    glBindVertexArray(triVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    if (program) glUseProgram(0);
}

// -----------------------------
// Mesh generators
// -----------------------------
Mesh* Renderer::createGrid(int size, float spacing) {
    Mesh* grid = new Mesh();
    grid->type = LINES;

    // subtle gray tint default
    grid->tint = {0.7f, 0.7f, 0.7f, 1.0f};

    for (int i = -size; i <= size; ++i) {
        float pos = i * spacing;

        // Vertical line (Z direction)
        Vertex v1{}, v2{};
        v1.position = { pos, 0.0f, -size * spacing };
        v2.position = { pos, 0.0f,  size * spacing };
        v1.normal = {0,1,0}; v2.normal = {0,1,0};
        v1.uv = {0,0}; v2.uv = {0,0};

        if (i == 0) {
            // X axis (red)
            v1.color[0] = 0.9f; v1.color[1] = 0.2f; v1.color[2] = 0.2f; v1.color[3] = 1.0f;
            v2.color[0] = 0.9f; v2.color[1] = 0.2f; v2.color[2] = 0.2f; v2.color[3] = 1.0f;
        } else {
            v1.color[0] = 0.6f; v1.color[1] = 0.6f; v1.color[2] = 0.6f; v1.color[3] = 1.0f;
            v2.color[0] = 0.6f; v2.color[1] = 0.6f; v2.color[2] = 0.6f; v2.color[3] = 1.0f;
        }

        uint32_t base = (uint32_t)grid->vertices.size();
        grid->vertices.push_back(v1);
        grid->vertices.push_back(v2);
        grid->indices.push_back(base + 0);
        grid->indices.push_back(base + 1);

        // Horizontal line (X direction)
        Vertex h1{}, h2{};
        h1.position = { -size * spacing, 0.0f, pos };
        h2.position = {  size * spacing, 0.0f, pos };
        h1.normal = {0,1,0}; h2.normal = {0,1,0};
        h1.uv = {0,0}; h2.uv = {0,0};

        if (i == 0) {
            // Z axis (blue)
            h1.color[0] = 0.2f; h1.color[1] = 0.2f; h1.color[2] = 0.9f; h1.color[3] = 1.0f;
            h2.color[0] = 0.2f; h2.color[1] = 0.2f; h2.color[2] = 0.9f; h2.color[3] = 1.0f;
        } else {
            h1.color[0] = 0.6f; h1.color[1] = 0.6f; h1.color[2] = 0.6f; h1.color[3] = 1.0f;
            h2.color[0] = 0.6f; h2.color[1] = 0.6f; h2.color[2] = 0.6f; h2.color[3] = 1.0f;
        }

        base = (uint32_t)grid->vertices.size();
        grid->vertices.push_back(h1);
        grid->vertices.push_back(h2);
        grid->indices.push_back(base + 0);
        grid->indices.push_back(base + 1);
    }

    return grid;
}

Mesh* Renderer::createCube(float size) {
    Mesh* cube = new Mesh();
    cube->type = TRIANGLES;

    float s = size * 0.5f;
    Vec3 pos[8] = {
        {-s,-s,-s},{ s,-s,-s},{ s, s,-s},{-s, s,-s},
        {-s,-s, s},{ s,-s, s},{ s, s, s},{-s, s, s}
    };

    int faceIdx[] = {
        0,1,2, 2,3,0, // back
        4,5,6, 6,7,4, // front
        0,4,7, 7,3,0, // left
        1,5,6, 6,2,1, // right
        3,2,6, 6,7,3, // top
        0,1,5, 5,4,0  // bottom
    };

    for (int i = 0; i < 36; ++i) {
        Vertex v{};
        Vec3 p = pos[faceIdx[i]];
        v.position = p;
        v.normal = {0,1,0};
        v.uv = {0,0};
        v.color[0] = 0.8f; v.color[1] = 0.8f; v.color[2] = 0.8f; v.color[3] = 1.0f;
        cube->vertices.push_back(v);
        cube->indices.push_back((uint32_t)i);
    }

    return cube;
}

// -----------------------------
// Loop control (emscripten)
// -----------------------------
#ifdef __EMSCRIPTEN__
static Renderer* s_loopRenderer = nullptr;
static void emscripten_main_loop(void*) {
    if (s_loopRenderer) s_loopRenderer->renderFrame();
}
#endif

void Renderer::startLoop() {
#ifdef __EMSCRIPTEN__
    s_loopRenderer = this;
    emscripten_set_main_loop_arg(emscripten_main_loop, nullptr, 0, 1);
#else
    running = true;
    // Native blocking loop (not ideal for real apps)
    while (running) {
        renderFrame();
        // naive sleep to avoid pegging CPU; platform-specific sleep could be used
#ifdef _WIN32
        Sleep(16);
#else
        struct timespec ts = {0, 16 * 1000000};
        nanosleep(&ts, nullptr);
#endif
    }
#endif
}

void Renderer::stopLoop() {
#ifdef __EMSCRIPTEN__
    emscripten_cancel_main_loop();
    s_loopRenderer = nullptr;
#else
    running = false;
#endif
}
