#include <emscripten/bind.h>
#include "core/renderer.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(mini_three) {
    class_<Renderer>("Renderer")
        .constructor<int,int>()
        .function("clear", &Renderer::clear);
}
