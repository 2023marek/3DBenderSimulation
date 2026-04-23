#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;
glm::mat4 getMVP()
{
    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 view = glm::lookAt(
        glm::vec3(200, 200, 200),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0)
    );

    glm::mat4 proj = glm::perspective(glm::radians(45.0f),
        WIDTH / (float)HEIGHT,
        0.1f, 1000.0f);

    return proj * view * model;
}
