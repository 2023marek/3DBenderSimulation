#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Core
#include "../Core/PipeAxis3D.h"

// Rendering
#include "../Render/PipeRenderer.h"
#include "../Render/ControlCamera.h"
#include "../Render/TubeMesh.h"
#include "../Render/RenderMode.h"

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
// HELPER
// =========================
void extractPoints(
    const PipeAxis3D& pipe,
    std::vector<Vec3D>& points)
{
    for (auto& n : pipe.getNodes())
        points.push_back(n.pos);
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

    double radius = 0.1;
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

    pipe.build(); //moved outside loop

    RenderMode mode = RenderMode::MESH;

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
            }
        }
        else keyPressed = false;

        // =========================
        // PREPARE DATA
        // =========================
        if (mode == RenderMode::LINE)
        {
            std::vector<float> vertices;

            for (auto& n : pipe.getNodes())
            {
                vertices.push_back((float)n.pos.x);
                vertices.push_back((float)n.pos.y);
                vertices.push_back((float)n.pos.z);
            }

            renderer.uploadLine(vertices);
        }
        else
        {
            std::vector<Vec3D> points;
            extractPoints(pipe, points);

            // FIX: use pipe data (not hardcoded centerline)
            mesh.generate(points, radius, segments);

            renderer.uploadMesh(
                mesh.getVertices(),
                mesh.getIndices()
            );
        }

        // =========================
        // RENDER
        // =========================
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.setMode(mode);
        renderer.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}