#include "renderer.h"
#include <iostream>

Renderer::Renderer(int width, int height) {
    std::cout << "Renderer initialized with size: "
              << width << "x" << height << std::endl;
}

void Renderer::clear(float r, float g, float b) {
    std::cout << "Clearing screen with color: "
              << r << ", " << g << ", " << b << std::endl;
}
