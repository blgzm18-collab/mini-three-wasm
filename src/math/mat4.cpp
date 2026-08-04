#include "mat4.h"
#include <cmath>
#include "mat3.h"

Mat4::Mat4() {
    for (int i = 0; i < 16; i++) data[i] = 0.0f;
    data[0] = data[5] = data[10] = data[15] = 1.0f;
}

Mat4 Mat4::identity() {
    Mat4 m;
    for (int i = 0; i < 16; i++) m.data[i] = 0.0f;
    m.data[0] = m.data[5] = m.data[10] = m.data[15] = 1.0f;
    return m;
}

Mat4 Mat4::translation(const Vec3& t) {
    Mat4 m = identity();
    m.data[12] = t.x;
    m.data[13] = t.y;
    m.data[14] = t.z;
    return m;
}

Mat4 Mat4::scale(const Vec3& s) {
    Mat4 m = identity();
    m.data[0] = s.x;
    m.data[5] = s.y;
    m.data[10] = s.z;
    return m;
}

Mat4 Mat4::rotationX(float a) {
    Mat4 m = identity();
    float c = cosf(a), s = sinf(a);
    m.data[5] = c;
    m.data[6] = s;
    m.data[9] = -s;
    m.data[10] = c;
    return m;
}

Mat4 Mat4::rotationY(float a) {
    Mat4 m = identity();
    float c = cosf(a), s = sinf(a);
    m.data[0] = c;
    m.data[2] = -s;
    m.data[8] = s;
    m.data[10] = c;
    return m;
}

Mat4 Mat4::rotationZ(float a) {
    Mat4 m = identity();
    float c = cosf(a), s = sinf(a);
    m.data[0] = c;
    m.data[1] = s;
    m.data[4] = -s;
    m.data[5] = c;
    return m;
}

Mat4 Mat4::operator*(const Mat4& o) const {
    Mat4 r;

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            r.data[row * 4 + col] =
                data[row * 4 + 0] * o.data[0 * 4 + col] +
                data[row * 4 + 1] * o.data[1 * 4 + col] +
                data[row * 4 + 2] * o.data[2 * 4 + col] +
                data[row * 4 + 3] * o.data[3 * 4 + col];
        }
    }

    return r;
}

Mat3 Mat4::toMat3() const {
    float m3[9] = {
        data[0], data[1], data[2],
        data[4], data[5], data[6],
        data[8], data[9], data[10]
    };
    return Mat3(m3);
}