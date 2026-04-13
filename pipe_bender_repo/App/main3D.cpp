#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Core
#include "../Core/PipeAxis3D.h"

// Rendering
#include "../Render/PipeRenderer.h"
#include "../Render/ControlCamera.h"
#include "../Render/TubeMesh.h"
#include "../Render/RenderMode.h"
#include "../Render/ShaderManager.h"  // ? USE MANAGER INSTEAD

// =========================
// CONSTANTS
// =========================
#ifndef PI
#define PI 3.14159265358979323846
#endif

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

// =========================
// GLOBAL CAMERA
// =========================
ControlCamera* gCamera = nullptr;

bool leftMousePressed = false;
bool rightMousePressed = false;
bool firstMouse = true;
double lastX = WIDTH / 2.0;
double lastY = HEIGHT / 2.0;

// =========================
// INPUT CALLBACKS
// =========================
void mouse_callback(GLFWwindow*, double xpos, double ypos)
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

void mouse_button_callback(GLFWwindow*, int button, int action, int)
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
// MAIN
// =========================
int main()
{
    // =========================
    // INIT GLFW
    // =========================
    if (!glfwInit())
    {
        std::cout << "Failed to init GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Pipe3D", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to init GLAD\n";
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    // =========================
    // SYSTEMS
    // =========================
    ControlCamera camera;
    gCamera = &camera;
    PipeRenderer renderer;
    TubeMesh mesh;

    // ? LOAD SHADERS VIA MANAGER
    ShaderGL* pipeShader = ShaderManager::instance().load(
        "pipe",
        "Source/ShaderFiles/pipe.vert",
        "Source/ShaderFiles/pipe.frag"
    );

    if (!pipeShader)
    {
        std::cerr << "Failed to load pipe shader\n";
        return -1;
    }

    double radius = 4.0;
    int segments = 16;

    // =========================
    // PIPE SETUP (build ONCE)
    // =========================
    PipeAxis3D pipe(5.0);
    pipe.addFeed(10);
    pipe.addBend(50, PI / 3);
    pipe.addRotate(PI / 3);
    pipe.addFeed(80);
    pipe.addBend(40, PI / 1);

    pipe.build();

    RenderMode mode = RenderMode::MESH;

    // =========================
    // GENERATE & UPLOAD MESH ONCE
    // =========================
    std::vector<Vec3D> points;
    std::vector<Vec3D> tangents;

    for (const auto& n : pipe.getNodes())
    {
        points.push_back(n.pos);
        tangents.push_back(n.T);
    }

    mesh.generate(points, tangents, radius, segments);
    std::cout << "Mesh generated: " << mesh.getVertices().size() << " vertices, "
        << mesh.getIndices().size() << " indices\n";

    renderer.uploadMesh(mesh.getVertices(), mesh.getIndices());

    // =========================
    // INPUT
    // =========================
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // =========================
    // MAIN LOOP
    // =========================
    while (!glfwWindowShouldClose(window))
    {
        // =========================
        // INPUT (mode toggle)
        // =========================
        static bool keyPressed = false;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            if (!keyPressed)
            {
                mode = (mode == RenderMode::LINE)
                    ? RenderMode::MESH
                    : RenderMode::LINE;

                keyPressed = true;
                std::cout << "Mode: " << (mode == RenderMode::LINE ? "LINE" : "MESH") << "\n";
            }
        }
        else keyPressed = false;

        // =========================
        // PREPARE DATA FOR RENDERING
        // =========================
        if (mode == RenderMode::LINE)
        {
            std::vector<float> vertices;

            for (const auto& n : pipe.getNodes())
            {
                vertices.push_back((float)n.pos.x);
                vertices.push_back((float)n.pos.y);
                vertices.push_back((float)n.pos.z);
            }

            renderer.uploadLine(vertices);
        }

        // =========================
        // RENDER
        // =========================
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.setMode(mode);
        pipeShader->use();  // ? USE SHADER FROM MANAGER

        // Camera matrices
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjection((float)WIDTH, (float)HEIGHT);

        glm::mat4 MVP = projection * view * model;

        // Set uniforms
        pipeShader->setMat4("MVP", MVP);
        pipeShader->setMat4("Model", model);

        // Normal matrix
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        pipeShader->setMat3("NormalMatrix", normalMatrix);

        // Lighting
        pipeShader->setVec3("lightPos", glm::vec3(10.0f, 10.0f, 10.0f));
        pipeShader->setVec3("viewPos", camera.getPosition());
        pipeShader->setVec3("objectColor", glm::vec3(0.2f, 0.8f, 0.3f));

        renderer.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}