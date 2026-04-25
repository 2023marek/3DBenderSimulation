#pragma once
#include <external/glm/glm/glm.hpp>

class Camera
{
public:
    glm::vec3 position;

    Camera();

    glm::mat4 getViewMatrix();
    glm::mat4 getProjection(float width, float height);
};