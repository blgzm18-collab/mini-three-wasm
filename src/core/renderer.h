// renderer.h
// Public header for the mini-three-wasm renderer
// Minimal, embind-friendly API surface used by bindings.cpp

#ifndef MINI_THREE_WASM_RENDERER_H
#define MINI_THREE_WASM_RENDERER_H

#include <vector>
#include <cstdint>
#include <array>

#ifdef __EMSCRIPTEN__
#include <emscripten/val.h>
#endif

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <GL/glew.h>
#endif

// Basic math types
struct Vec3 { float x, y, z; };
struct Vec2 { float u, v; };

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    float color[4];
};

enum PrimitiveType {
    TRIANGLES = 0,
    LINES = 1
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    PrimitiveType type = TRIANGLES;

    // GPU handles (managed by renderer)
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    bool uploaded = false;

    // tint stored as std::array to avoid C-array binding issues
    std::array<float,4> tint = {0.8f, 0.8f, 0.8f, 1.0f};

    Mesh() = default;
    ~Mesh() = default;

    // Embind-friendly accessors (implemented in renderer.cpp)
#ifdef __EMSCRIPTEN__
    emscripten::val getTint() const;
    void setTint(const emscripten::val& arr);
#else
    std::array<float,4> getTint() const { return tint; }
    void setTint(const std::array<float,4>& t) { tint = t; }
#endif
};

// Forward declaration
class Renderer {
public:
    Renderer();
    Renderer(int width, int height);
    ~Renderer();

    // Initialize GL state and shaders
    bool init();

    // Simple helpers used by bindings/tests
    void clear(float r, float g, float b);
    void drawTriangle();

    // Scene management
    void addMesh(Mesh* mesh);
    void renderFrame();

    // Mesh generators
    Mesh* createGrid(int size, float spacing);
    Mesh* createCube(float size);

    // Optional loop control (no-op for embind usage; provided for completeness)
    void startLoop();
    void stopLoop();

private:
    // Internal helpers
    bool compileShaders();
    void uploadMesh(Mesh* mesh);
    void drawMesh(Mesh* mesh);

    // GL/program state
    GLuint program = 0;
    GLint uProjViewLoc = -1;
    GLint uModelLoc = -1;
    GLint uColorLoc = -1;

    float projView[16];
    bool running = false;

    // Scene storage
    std::vector<Mesh*> sceneMeshes;
};

#endif // MINI_THREE_WASM_RENDERER_H
