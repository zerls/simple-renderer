#pragma once
#include "maths.h"

class Camera {
public:
    Camera();
    void setPosition(const Vec3f& position);
    void setTarget(const Vec3f& target);
    void setUp(const Vec3f& up);
    void setFOV(float fov);
    void setAspect(float aspect);
    void setNearPlane(float nearPlane);
    void setFarPlane(float farPlane);

    Vec3f getPosition() const;
    Vec3f getTarget() const;
    Vec3f getUp() const;
    float getFOV() const;
    float getAspect() const;
    float getNearPlane() const;
    float getFarPlane() const;

    Matrix4x4f getViewMatrix() const;
    Matrix4x4f getProjectionMatrix() const;

private:
    Vec3f position;
    Vec3f target;
    Vec3f up;
    float fov;
    float aspect;
    float nearPlane;
    float farPlane;
};