#pragma once
#include "vec3.h"
#include "mat3.h"

class Mat4 {
public:
    float data[16];

    Mat4(); // identity
    Mat3 toMat3() const;

    static Mat4 identity();
    static Mat4 translation(const Vec3& t);
    static Mat4 scale(const Vec3& s);
    static Mat4 rotationX(float angle);
    static Mat4 rotationY(float angle);
    static Mat4 rotationZ(float angle);

    Mat4 operator*(const Mat4& other) const;
};
