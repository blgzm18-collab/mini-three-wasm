#include "camera.h"
#include <cmath>

Camera::Camera()
    : position(0, 0, 3), rotation(0, 0, 0) {}

Mat4 Camera::getViewMatrix() const {
    Mat4 T = Mat4::translation(Vec3(-position.x, -position.y, -position.z));
    Mat4 RX = Mat4::rotationX(-rotation.x);
    Mat4 RY = Mat4::rotationY(-rotation.y);
    Mat4 RZ = Mat4::rotationZ(-rotation.z);

    return RZ * RY * RX * T;
}

Mat4 Camera::getProjectionMatrix(float aspect) const {
    float fov = 60.0f * (3.14159f / 180.0f);
    float near = 0.1f;
    float far = 100.0f;

    float f = 1.0f / tanf(fov / 2.0f);

    Mat4 m;
    m.data[0] = f / aspect;
    m.data[5] = f;
    m.data[10] = (far + near) / (near - far);
    m.data[11] = -1.0f;
    m.data[14] = (2.0f * far * near) / (near - far);
    m.data[15] = 0.0f;

    return m;
}
