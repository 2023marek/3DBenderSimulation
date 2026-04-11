#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Core
#include "../Core/PipeAxis3D.h"

// Rendering
#include "../Render/PipeRenderer.h"
#include "../Render/ControlCamera.h"
#include "../Render/TubeMesh.h"
#include "../Render/RenderMode.h" // <-- shared enum

// =========================
// WINDOW CONFIG
// =========================
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

// =========================
// GLOBAL CAMERA (INPUT SYSTEM)
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
        gCamera->processMouseMovement(dx, -dy); // rotate

    if (rightMousePressed)
        gCamera->processPan(dx, dy); // pan
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
// HELPER: extract tangents
// =========================
void extractPointsAndTangents(
    const PipeAxis3D& pipe,
    std::vector<Vec3D>& points,
    std::vector<Vec3D>& tangents)
{
    const auto& nodes = pipe.getNodes();

    for (size_t i = 0; i < nodes.size(); i++)
    {
        points.push_back(nodes[i].pos);

        if (i < nodes.size() - 1)
        {
            Vec3D t = nodes[i + 1].pos - nodes[i].pos;
            tangents.push_back(normalize(t));
        }
        else
        {
            tangents.push_back(tangents.back());
        }
    }
}

// =========================
// MAIN
// =========================
int main()
{
    // =========================================
    // INIT WINDOW (Platform Layer)
    // =========================================
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Pipe3D", NULL, NULL);
    glfwMakeContextCurrent(window);

    gladLoadGL();
    glEnable(GL_DEPTH_TEST);

    // =========================================
    // CREATE SYSTEMS (Engine Initialization)
    // =========================================

    ControlCamera camera;
    gCamera = &camera;

    PipeRenderer renderer;     // NOW USED PROPERLY
    TubeMesh tube(5.0, 16);   // mesh generator

    // Pipe logic (Scene)
    PipeAxis3D pipe(5.0);
    pipe.addFeed(10);
    pipe.addBend(50, PI / 3);
    pipe.addRotate(PI / 3);
    pipe.addFeed(80);
    pipe.addBend(40, PI / 1);

    RenderMode mode = RenderMode::MESH;

    // Input hooks
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    

    while (!glfwWindowShouldClose(window))
    {
        // =========================
        // INPUT (mode switching)
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
        // UPDATE (Scene logic)
        // =========================
        pipe.build();

        // =========================
        // PREPARE DATA FOR RENDERER
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
            std::vector<Vec3D> points, tangents;
            extractPointsAndTangents(pipe, points, tangents);

            tube.build(points, tangents);

            renderer.uploadMesh(
                tube.getVertices(),
                tube.getNormals(),
                tube.getIndices()
            );
        }

        // =========================
        // RENDER
        // =========================
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.setMode(mode);   // <-- clean API
        renderer.draw();          // <-- ONLY draw call

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}