#include "../Render/CameraGL.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
{
    position = glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(
        position,
        position + glm::vec3(0.0f, 0.0f, -1.0f), // forward
        glm::vec3(0.0f, 1.0f, 0.0f)              // up
    );
}

glm::mat4 Camera::getProjection(float width, float height)
{
    return glm::perspective(
        glm::radians(45.0f),
        width / height,
        0.1f,
        100.0f
    );
}