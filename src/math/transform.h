#pragma once
#include "mat4.h"
#include "vec3.h"

class Transform {
public:
    Vec3 position;
    Vec3 rotation; // Euler angles in radians
    Vec3 scale;

    Transform()
        : position(0,0,0), rotation(0,0,0), scale(1,1,1) {}

    Mat4 getModelMatrix() const;
};
