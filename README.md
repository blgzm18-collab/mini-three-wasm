# mini-three-wasm  
A lightweight experimental **3D rendering engine for the browser**, inspired by frameworks like **three.js** and **Babylon.js** — except this one is written in **C++**, compiled to **WebAssembly**, and exists purely because I got bored.  
Don’t judge. It works.

## ✨ Overview  
`mini-three-wasm` is a small but growing WebGL2 renderer built with:

- A C++ core (renderer, shader system, mesh pipeline, camera)
- A math library (vec3, mat3, mat4, transforms)
- Embind bindings for JavaScript → WASM communication
- A modular structure designed to scale into a real engine

The goal is to understand how far a custom WASM-based renderer can go while staying minimal, fast, and fun.

---

## 🔧 Build Script  
Compile the engine using Emscripten:

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
  -O3 -lembind -s WASM=1 -s USE_WEBGL2=1 \
  -s MODULARIZE=1 -s EXPORT_ES6=1 -s ALLOW_MEMORY_GROWTH=1 \
  -o web/engine.js
```

This produces:

- `engine.js` — ES6 module wrapper  
- `engine.wasm` — compiled WebAssembly  
- A renderer ready to be imported into your browser environment

---

## 🧩 Current Features  
- Basic WebGL2 context + renderer loop  
- Shader compilation + linking  
- Mesh + vertex buffer abstraction  
- Perspective camera  
- Minimal math library (vec3, mat3, mat4, transforms)  
- Embind bindings for JS interop  
- WASM memory growth enabled for future expansion  

---

## 🚀 Future Direction  
This repo is the foundation for a **mini three.js‑style engine**, but the long-term plan is bigger:

### Engine Roadmap  
- Add **material system** (Phong, PBR-lite, custom shaders)  
- Implement **scene graph** + hierarchical transforms  
- Add **geometry generators** (cube, sphere, plane, torus)  
- Introduce **texture loading** + WebGL2 sampler support  
- Add **framebuffer system** for post-processing  
- Build a **WASM-friendly asset pipeline**  
- Create a **JS wrapper API** that feels like three.js but lighter  

### Why this repo exists  
This project is meant to become:

- A **learning playground** for C++ → WASM rendering  
- A **testbed** for custom engine architecture  
- A **future internal renderer** for Vantari Studios projects  
- A **foundation** for experimenting with real-time graphics in the browser  
- A **mini-engine** that can eventually load models, scenes, and materials  

### Long-term vision  
The renderer may evolve into:

- A standalone **WASM graphics SDK**  
- A modular engine powering internal tools  
- A lightweight alternative to three.js for custom workflows  
- A foundation for future Vantari Studios web-based games or tools  

---

## 📁 Project Structure  
```
mini-three-wasm/
  ├─ src/
  │   ├─ core/        # renderer, shader, mesh, camera
  │   ├─ math/        # vec3, mat3, mat4, transforms
  │   ├─ bindings/    # embind → JS interface
  │   └─ main.cpp     # entry point
  │
  ├─ web/             # browser-facing files
  │   ├─ index.html
  │   ├─ engine.js
  │   └─ styles.css
  │
  ├─ build/           # Emscripten output
  └─ .gitignore
```

---

## 🔒 Status  
This project is experimental, evolving, and not intended for public use.  
It’s a playground — but a serious one.
