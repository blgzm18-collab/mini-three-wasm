// renderer.cpp
// Minimal C++ renderer for WebAssembly (Emscripten + WebGL2)
// Features:
// - Mesh struct with TRIANGLES and LINES
// - Grid generator (geometry-based)
// - Simple shader pipeline for colored meshes
// - Upload to GPU (VBO/IBO/VAO) and draw path
// - Exports for WASM: initRenderer, createGridExport, createCubeExport, startRenderLoop, stopRenderLoop

#include <vector>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#else
// For native testing you may need to include GL headers appropriate to your platform
#include <GL/glew.h>
#endif

// -----------------------------
// Basic math types
// -----------------------------
struct Vec3 {
    float x, y, z;
};

struct Vec2 {
    float u, v;
};

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    float color[4];
};

// -----------------------------
// Primitive type
// -----------------------------
enum PrimitiveType {
    TRIANGLES = 0,
    LINES = 1
};

// -----------------------------
// Mesh representation
// -----------------------------
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    PrimitiveType type = TRIANGLES;

    // GPU handles
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    bool uploaded = false;
    float tint[4] = {0.8f, 0.8f, 0.8f, 1.0f};
};

// -----------------------------
// Renderer class
// -----------------------------
class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init();
    void addMesh(Mesh* mesh);
    void renderFrame();
    Mesh* createGrid(int size, float spacing);
    Mesh* createCube(float size);

    // Simple scene storage
    std::vector<Mesh*> sceneMeshes;

private:
    GLuint program = 0;
    GLint uProjViewLoc = -1;
    GLint uModelLoc = -1;
    GLint uColorLoc = -1;

    bool compileShaders();
    void uploadMesh(Mesh* mesh);
    void drawMesh(Mesh* mesh);

    // Simple projection/view (orthographic for grid demo)
    float projView[16];
    bool running = false;
};

static Renderer* g_renderer = nullptr;

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
out vec3 vWorldPos;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    gl_Position = uProjView * worldPos;
    vWorldPos = worldPos.xyz;
    vNormal = mat3(uModel) * aNormal;
    vColor = aColor;
}
)";

static const char* fragmentShaderSrc = R"(
#version 300 es
precision highp float;

in vec4 vColor;
in vec3 vNormal;
in vec3 vWorldPos;

uniform vec4 uTint;

out vec4 outColor;

void main() {
    // Simple lambert-ish lighting with ambient + directional
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float NdotL = max(dot(normalize(vNormal), lightDir), 0.0);
    vec3 ambient = vec3(0.12);
    vec3 diffuse = vec3(0.88) * NdotL;
    vec3 color = (ambient + diffuse) * vColor.rgb * uTint.rgb;
    outColor = vec4(color, vColor.a * uTint.a);
}
)";

// -----------------------------
// Utility: compile shader
// -----------------------------
static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, nullptr, &log[0]);
        std::cerr << "Shader compile error: " << log << std::endl;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
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
Renderer::Renderer() {}

Renderer::~Renderer() {
    for (auto m : sceneMeshes) {
        if (m) {
            if (m->uploaded) {
                glDeleteBuffers(1, &m->vbo);
                glDeleteBuffers(1, &m->ibo);
                glDeleteVertexArrays(1, &m->vao);
            }
            delete m;
        }
    }
    if (program) glDeleteProgram(program);
}

bool Renderer::compileShaders() {
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
    if (!compileShaders()) return false;

    // Basic GL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup a simple orthographic projView for demo
    // Column-major order
    float left = -10.0f, right = 10.0f, bottom = -6.0f, top = 6.0f, nearp = -50.0f, farp = 50.0f;
    float rl = 1.0f / (right - left);
    float tb = 1.0f / (top - bottom);
    float fn = 1.0f / (farp - nearp);

    // ortho matrix
    memset(projView, 0, sizeof(projView));
    projView[0] = 2.0f * rl;
    projView[5] = 2.0f * tb;
    projView[10] = -2.0f * fn;
    projView[12] = -(right + left) * rl;
    projView[13] = -(top + bottom) * tb;
    projView[14] = -(farp + nearp) * fn;
    projView[15] = 1.0f;

    return true;
}

void Renderer::uploadMesh(Mesh* mesh) {
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
    // location 0: position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, position));
    // location 1: normal (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, normal));
    // location 2: uv (vec2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, uv));
    // location 3: color (vec4)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, color));

    glBindVertexArray(0);

    mesh->uploaded = true;
}

void Renderer::drawMesh(Mesh* mesh) {
    if (!mesh->uploaded) uploadMesh(mesh);

    glUseProgram(program);

    // Model matrix identity for now
    float model[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };

    glUniformMatrix4fv(uProjViewLoc, 1, GL_FALSE, projView);
    glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, model);
    glUniform4fv(uColorLoc, 1, mesh->tint);

    glBindVertexArray(mesh->vao);

    GLenum mode = (mesh->type == LINES) ? GL_LINES : GL_TRIANGLES;
    glDrawElements(mode, (GLsizei)mesh->indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::renderFrame() {
    glViewport(0, 0, 1024, 768);
    glClearColor(0.12f, 0.14f, 0.16f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto m : sceneMeshes) {
        if (m) drawMesh(m);
    }
}

void Renderer::addMesh(Mesh* mesh) {
    sceneMeshes.push_back(mesh);
}

// -----------------------------
// Grid generator
// -----------------------------
Mesh* Renderer::createGrid(int size, float spacing) {
    Mesh* grid = new Mesh();
    grid->type = LINES;

    // grid color: subtle gray
    grid->tint[0] = 0.7f;
    grid->tint[1] = 0.7f;
    grid->tint[2] = 0.7f;
    grid->tint[3] = 1.0f;

    // center lines and axis lines
    for (int i = -size; i <= size; ++i) {
        float pos = i * spacing;

        // Vertical line (along Z)
        Vertex v1, v2;
        v1.position = { pos, 0.0f, -size * spacing };
        v2.position = { pos, 0.0f,  size * spacing };

        // normals and uv not important for grid
        v1.normal = {0,1,0};
        v2.normal = {0,1,0};
        v1.uv = {0,0};
        v2.uv = {0,0};

        // color: slightly darker for center axis
        if (i == 0) {
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

        // Horizontal line (along X)
        Vertex h1, h2;
        h1.position = { -size * spacing, 0.0f, pos };
        h2.position = {  size * spacing, 0.0f, pos };
        h1.normal = {0,1,0};
        h2.normal = {0,1,0};
        h1.uv = {0,0};
        h2.uv = {0,0};

        if (i == 0) {
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

// -----------------------------
// Simple cube generator for testing
// -----------------------------
Mesh* Renderer::createCube(float size) {
    Mesh* cube = new Mesh();
    cube->type = TRIANGLES;

    float s = size * 0.5f;

    // 8 positions
    Vec3 pos[8] = {
        {-s,-s,-s},{ s,-s,-s},{ s, s,-s},{-s, s,-s},
        {-s,-s, s},{ s,-s, s},{ s, s, s},{-s, s, s}
    };

    // simple cube faces (12 triangles)
    int faceIdx[] = {
        0,1,2, 2,3,0, // back
        4,5,6, 6,7,4, // front
        0,4,7, 7,3,0, // left
        1,5,6, 6,2,1, // right
        3,2,6, 6,7,3, // top
        0,1,5, 5,4,0  // bottom
    };

    for (int i = 0; i < 36; ++i) {
        Vertex v;
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
// Global C API for WASM
// -----------------------------
extern "C" {

// Initialize renderer and create default scene
bool initRenderer() {
    if (g_renderer) return true;
    g_renderer = new Renderer();
    bool ok = g_renderer->init();
    if (!ok) {
        delete g_renderer;
        g_renderer = nullptr;
        return false;
    }

    // Create default grid and cube
    Mesh* grid = g_renderer->createGrid(20, 1.0f);
    g_renderer->addMesh(grid);

    Mesh* cube = g_renderer->createCube(1.0f);
    // Move cube up a bit by modifying vertex positions
    for (auto &v : cube->vertices) v.position.y += 0.5f;
    cube->tint[0] = 0.9f; cube->tint[1] = 0.7f; cube->tint[2] = 0.3f; cube->tint[3] = 1.0f;
    g_renderer->addMesh(cube);

    return true;
}

// Create a grid and return pointer (opaque) to mesh
Mesh* createGridExport(int size, float spacing) {
    if (!g_renderer) return nullptr;
    Mesh* grid = g_renderer->createGrid(size, spacing);
    g_renderer->addMesh(grid);
    return grid;
}

// Create a cube and return pointer
Mesh* createCubeExport(float size) {
    if (!g_renderer) return nullptr;
    Mesh* cube = g_renderer->createCube(size);
    g_renderer->addMesh(cube);
    return cube;
}

// Start render loop (Emscripten)
#ifdef __EMSCRIPTEN__
static void emscripten_frame() {
    if (g_renderer) g_renderer->renderFrame();
}
#endif

void startRenderLoop() {
    if (!g_renderer) return;
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(emscripten_frame, 0, 1);
#else
    // Native fallback: simple loop (blocking)
    while (true) {
        g_renderer->renderFrame();
        // Sleep or break condition should be added for native builds
    }
#endif
}

