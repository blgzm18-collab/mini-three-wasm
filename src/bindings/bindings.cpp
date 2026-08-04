// bindings.cpp
// Embind bindings + C wrappers for the mini-three-wasm renderer

#include "../core/renderer.h"
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

using namespace emscripten;

// -----------------------------
// C ABI wrappers (optional)
// -----------------------------
extern "C" {

Renderer* renderer_create(int w, int h) {
    Renderer* r = new Renderer(w, h);
    if (!r->init()) {
        delete r;
        return nullptr;
    }
    return r;
}

void renderer_destroy(Renderer* r) {
    if (!r) return;
    delete r;
}

void renderer_clear(Renderer* r, float rr, float gg, float bb) {
    if (!r) return;
    r->clear(rr, gg, bb);
}

void renderer_drawTriangle(Renderer* r) {
    if (!r) return;
    r->drawTriangle();
}

Mesh* renderer_createGrid(Renderer* r, int size, float spacing) {
    if (!r) return nullptr;
    Mesh* m = r->createGrid(size, spacing);
    r->addMesh(m);
    return m;
}

Mesh* renderer_createCube(Renderer* r, float size) {
    if (!r) return nullptr;
    Mesh* m = r->createCube(size);
    r->addMesh(m);
    return m;
}

void renderer_renderFrame(Renderer* r) {
    if (!r) return;
    r->renderFrame();
}

} // extern "C"


// -----------------------------
// Embind bindings
// -----------------------------
EMSCRIPTEN_BINDINGS(mini_three_wasm) {

    // Vec2
    value_object<Vec2>("Vec2")
        .field("u", &Vec2::u)
        .field("v", &Vec2::v);

    // Vec3
    value_object<Vec3>("Vec3")
        .field("x", &Vec3::x)
        .field("y", &Vec3::y)
        .field("z", &Vec3::z);

    // Vertex
    value_object<Vertex>("Vertex")
        .field("position", &Vertex::position)
        .field("normal", &Vertex::normal)
        .field("uv", &Vertex::uv)
        .field("color", &Vertex::color);

    // Mesh: expose constructor, type, and tint accessors
    class_<Mesh>("Mesh")
        .constructor<>()
        .function("getTint", &Mesh::getTint)
        .function("setTint", &Mesh::setTint)
        .property("type", &Mesh::type);

    // Renderer
    class_<Renderer>("Renderer")
        .constructor<>()
        .constructor<int,int>()
        .function("init", &Renderer::init)
        .function("clear", &Renderer::clear)
        .function("drawTriangle", &Renderer::drawTriangle)
        .function("addMesh", &Renderer::addMesh, allow_raw_pointers())
        .function("renderFrame", &Renderer::renderFrame)
        .function("createGrid", &Renderer::createGrid, allow_raw_pointers())
        .function("createCube", &Renderer::createCube, allow_raw_pointers())
        .function("startLoop", &Renderer::startLoop)
        .function("stopLoop", &Renderer::stopLoop);

    // Expose C wrappers as functions for convenience
    function("renderer_create", &renderer_create, allow_raw_pointers());
    function("renderer_destroy", &renderer_destroy, allow_raw_pointers());
    function("renderer_clear", &renderer_clear, allow_raw_pointers());
    function("renderer_drawTriangle", &renderer_drawTriangle, allow_raw_pointers());
    function("renderer_createGrid", &renderer_createGrid, allow_raw_pointers());
    function("renderer_createCube", &renderer_createCube, allow_raw_pointers());
    function("renderer_renderFrame", &renderer_renderFrame, allow_raw_pointers());
}
