#pragma once
#include "../math/mat4.h"
#include "../math/vec3.h"

class Camera {
public:
    Vec3 position;
    Vec3 rotation; // pitch, yaw, roll

    Camera();

    Mat4 getViewMatrix() const;
    Mat4 getProjectionMatrix(float aspect) const;
};
