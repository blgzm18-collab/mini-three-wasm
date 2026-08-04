# mini-three-wasm

A 3d rendering tool for the browser similar to three.js or babylon.js but made using C++ for some reason. I got bored dont judge.

## Build script :

```
emcc \
  src/core/renderer.cpp \
  src/core/shader.cpp \
  src/core/mesh.cpp \
  src/core/camera.cpp \
  src/math/mat4.cpp \
  src/math/vec3.cpp \
  src/math/transform.cpp \
  src/math/mat3.cpp \
  src/bindings/bindings.cpp \
  -O3 -lembind -s WASM=1 -s USE_WEBGL2=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ALLOW_MEMORY_GROWTH=1 \
  -o web/engine.js

```