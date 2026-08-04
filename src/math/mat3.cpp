// src/math/mat3.cpp
#include "mat3.h"

Mat3::Mat3() {
    for (int i = 0; i < 9; i++) data[i] = 0.0f;
}

Mat3::Mat3(const float* src) {
    for (int i = 0; i < 9; i++) data[i] = src[i];
}
