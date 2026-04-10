#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <iostream>

#include "../Core/PipeAxis3D.h"
#include "../Render/PipeRenderer.h"
#include "../Render/ShaderGL.h"
#include "../Render/ControlCamera.h"
#include "../Render/TubeMesh.h"

// =========================
// WINDOW
// =========================
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

// =========================
// MOUSE STATE
// =========================
double lastX = WIDTH / 2.0;
double lastY = HEIGHT / 2.0;

bool firstMouse = true;
bool leftMousePressed = false;
bool rightMousePressed = false;

ControlCamera* gCamera = nullptr;

// =========================
// SHADERS
// =========================
const char* vertexSrc = R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 MVP;
uniform mat4 model;

out vec3 FragPos;
out vec3 Normal;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(model) * aNormal;

    gl_Position = MVP * vec4(aPos, 1.0);
}
)";

const char* fragmentSrc = R"(
#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);

    vec3 color = vec3(0.2, 0.8, 0.3) * diff;

    FragColor = vec4(color, 1.0);
}
)";

// =========================
// CALLBACKS
// =========================
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float dx = (float)(xpos - lastX);
    float dy = (float)(lastY - ypos);

    lastX = xpos;
    lastY = ypos;

    if (!gCamera) return;

    if (leftMousePressed)
        gCamera->processMouseMovement(dx, -dy);

    if (rightMousePressed)
        gCamera->processPan(dx, dy);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
        leftMousePressed = (action == GLFW_PRESS);

    if (button == GLFW_MOUSE_BUTTON_RIGHT)
        rightMousePressed = (action == GLFW_PRESS);
}

void scroll_callback(GLFWwindow*, double, double yoffset)
{
    if (gCamera)
        gCamera->processScroll((float)yoffset);
}

// =========================
// MVP
// =========================
glm::mat4 getMVP(ControlCamera& camera)
{
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjection(WIDTH, HEIGHT);

    return proj * view * model;
}

// =========================
// MAIN
// =========================
int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Pipe3D", NULL, NULL);
    glfwMakeContextCurrent(window);

    gladLoadGL();
    glEnable(GL_DEPTH_TEST);

    // =========================
    // SYSTEMS
    // =========================
    ShaderGL shader(vertexSrc, fragmentSrc);
    PipeRenderer renderer;
    TubeMesh tube(5.0, 16);

    PipeAxis3D pipe(5.0);

    pipe.addFeed(10);
    pipe.addBend(50, PI / 3);
    pipe.addRotate(PI / 3);
    pipe.addFeed(80);
    pipe.addBend(40, PI / 1);

    ControlCamera camera;
    gCamera = &camera;

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    RenderMode mode = RenderMode::MESH;

    // =========================
    // LOOP
    // =========================
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        pipe.build();

        // =========================
        // BUILD GEOMETRY
        // =========================
        std::vector<Vec3D> points;
        std::vector<Vec3D> tangents;

        for (const auto& n : pipe.getNodes())
            points.push_back(n.pos);

        for (size_t i = 0; i + 1 < points.size(); i++)
        {
            Vec3D t = points[i + 1] - points[i];
            tangents.push_back(normalize(t));
        }

        if (!tangents.empty())
            tangents.push_back(tangents.back());

        tube.build(points, tangents);

        // =========================
        // INPUT (MODE SWITCH)
        // =========================
        static bool keyPressed = false;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            if (!keyPressed)
            {
                mode = (mode == RenderMode::LINE) ? RenderMode::MESH : RenderMode::LINE;
                keyPressed = true;

                std::cout << "Mode: "
                    << (mode == RenderMode::LINE ? "LINE" : "MESH")
                    << "\n";
            }
        }
        else
        {
            keyPressed = false;
        }

        // =========================
        // SHADER
        // =========================
        shader.use();

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 MVP = getMVP(camera);

        shader.setMat4("MVP", MVP);
        shader.setMat4("model", model);

        shader.setVec3("lightPos", glm::vec3(10, 10, 10));
        shader.setVec3("viewPos", camera.getPosition());

        // =========================
        // RENDER
        // =========================
        renderer.setMode(mode);

        if (mode == RenderMode::MESH)
        {
            renderer.uploadMesh(
                tube.getVertices(),
                tube.getNormals(),
                tube.getIndices()
            );
        }
        else
        {
            // build line vertices
            std::vector<float> lineVertices;

            for (const auto& n : pipe.getNodes())
            {
                lineVertices.push_back((float)n.pos.x);
                lineVertices.push_back((float)n.pos.y);
                lineVertices.push_back((float)n.pos.z);
            }

            renderer.uploadLine(lineVertices);
        }

        renderer.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}