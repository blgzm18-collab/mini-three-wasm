#include "transform.h"
#include <cmath>

Mat4 Transform::getModelMatrix() const {
    Mat4 T = Mat4::translation(position);
    Mat4 RX = Mat4::rotationX(rotation.x);
    Mat4 RY = Mat4::rotationY(rotation.y);
    Mat4 RZ = Mat4::rotationZ(rotation.z);
    Mat4 S = Mat4::scale(scale);

    return T * RZ * RY * RX * S;
}
