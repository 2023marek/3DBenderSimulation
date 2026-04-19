#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <iomanip>

// =========================================================================
// PHASE 4B: SIMULATION + RENDERING + ENHANCED HUD
// =========================================================================
//
// PURPOSE: Connect SimulationController to OpenGL rendering loop
//          with real-time on-screen HUD visualization
//
// ARCHITECTURE:
//   1. Load program into SimulationController
//   2. Each frame:
//      - Update simulation (calculate progress)
//      - Rebuild geometry (PipeAxis3D)
//      - Generate mesh (TubeMesh)
//      - Update HUD data
//      - Render 3D scene
//      - Render 2D HUD overlay
//   3. User controls: Play/Pause/Reset via keyboard
//
// =========================================================================

// Core
#include "../Core/PipeAxis3D.h"
#include "../Core/ProgramLoader.h"
#include "../Core/OperationQueue.h"
#include "../Core/SimulationController.h"
#include "../Machine/MachineState.h"

// Rendering
#include "../Render/PipeRenderer.h"
#include "../Render/ControlCamera.h"
#include "../Render/TubeMesh.h"
#include "../Render/RenderMode.h"
#include "../Render/ShaderManager.h"
#include "../Render/HUDPanel.h"  // ? NEW: HUD Panel

// =========================================================================
// CONSTANTS & GLOBALS
// =========================================================================

#ifndef PI
#define PI 3.14159265358979323846
#endif

const unsigned int WIDTH = 1100;
const unsigned int HEIGHT = 700;

// =========================================================================
// GLOBAL STATE
// =========================================================================

ControlCamera* gCamera = nullptr;
SimulationController* gSimulator = nullptr;
HUDPanel* gHUD = nullptr;  // ? NEW: HUD pointer

// Mouse input
bool leftMousePressed = false;
bool rightMousePressed = false;
bool firstMouse = true;
double lastX = WIDTH / 2.0;
double lastY = HEIGHT / 2.0;

// Keyboard debouncing
bool keyPlayPressed = false;
bool keyPausePressed = false;
bool keyResetPressed = false;
bool keyModePressed = false;
bool keyHUDToggle = false;  // ? NEW: HUD toggle

// Frame timing
std::chrono::high_resolution_clock::time_point lastFrameTime;
double deltaTime = 0.0;

// =========================================================================
// INPUT CALLBACKS
// =========================================================================

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

// =========================================================================
// KEYBOARD INPUT PROCESSING
// =========================================================================

void processKeyboardInput(GLFWwindow* window, RenderMode& mode)
{
    if (!gSimulator || !gHUD) return;

    // P: PLAY
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        if (!keyPlayPressed)
        {
            gSimulator->play();
            keyPlayPressed = true;
            std::cout << "\n? PLAY\n";
        }
    }
    else
    {
        keyPlayPressed = false;
    }

    // SPACE: PAUSE / RESUME
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        if (!keyPausePressed)
        {
            if (gSimulator->isPlaying())
            {
                gSimulator->pause();
                std::cout << "\n? PAUSE\n";
            }
            else if (!gSimulator->isPaused())
            {
                gSimulator->play();
                std::cout << "\n? RESUME\n";
            }
            keyPausePressed = true;
        }
    }
    else
    {
        keyPausePressed = false;
    }

    // R: RESET
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        if (!keyResetPressed)
        {
            gSimulator->reset();
            keyResetPressed = true;
            std::cout << "\n? RESET\n";
        }
    }
    else
    {
        keyResetPressed = false;
    }

    // M: TOGGLE RENDER MODE
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
        if (!keyModePressed)
        {
            mode = (mode == RenderMode::LINE)
                ? RenderMode::MESH
                : RenderMode::LINE;

            keyModePressed = true;
            std::cout << "? Mode: " << (mode == RenderMode::LINE ? "LINE" : "MESH") << "\n";
        }
    }
    else
    {
        keyModePressed = false;
    }

    // H: TOGGLE HUD VISIBILITY (? NEW)
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
    {
        if (!keyHUDToggle)
        {
            gHUD->setVisible(!gHUD->isVisible());
            keyHUDToggle = true;
            std::cout << "? HUD: " << (gHUD->isVisible() ? "ON" : "OFF") << "\n";
        }
    }
    else
    {
        keyHUDToggle = false;
    }

    // +/-: ADJUST SPEED
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
    {
        gSimulator->setSpeed(gSimulator->getSpeed() * 1.05);
    }

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
    {
        gSimulator->setSpeed(gSimulator->getSpeed() / 1.05);
    }
}

// =========================================================================
// MAIN
// =========================================================================

int main()
{
    // =====================================================================
    // INITIALIZATION PHASE
    // =====================================================================

    std::cout << "\n";
    std::cout << "?????????????????????????????????????????????????????????????????\n";
    std::cout << "?         PHASE 4B: SIMULATION + ENHANCED HUD DISPLAY           ?\n";
    std::cout << "?                                                               ?\n";
    std::cout << "?  Controls:                                                    ?\n";
    std::cout << "?    [P]      = Play                                            ?\n";
    std::cout << "?    [Space]  = Pause / Resume                                  ?\n";
    std::cout << "?    [R]      = Reset                                           ?\n";
    std::cout << "?    [M]      = Toggle render mode (LINE/MESH)                  ?\n";
    std::cout << "?    [H]      = Toggle HUD visibility                           ?\n";
    std::cout << "?    [+/-]    = Adjust speed                                    ?\n";
    std::cout << "?    [LMB]    = Rotate camera                                   ?\n";
    std::cout << "?    [RMB]    = Pan camera                                      ?\n";
    std::cout << "?    [Scroll] = Zoom camera                                     ?\n";
    std::cout << "?????????????????????????????????????????????????????????????????\n\n";

    // =====================================================================
    // GLFW INITIALIZATION
    // =====================================================================

    if (!glfwInit())
    {
        std::cerr << "? Failed to initialize GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "Pipe Bender Simulator - Phase 4B", NULL, NULL);
    if (!window)
    {
        std::cerr << "? Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "? Failed to initialize GLAD\n";
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    std::cout << "? OpenGL initialized\n";

    // =====================================================================
    // CAMERA SETUP
    // =====================================================================

    ControlCamera camera;
    gCamera = &camera;

    std::cout << "? Camera created\n";

    // =====================================================================
    // RENDERER SETUP
    // =====================================================================

    PipeRenderer renderer;
    TubeMesh mesh;
    double radius = 8.0;
    int segments = 16;

    std::cout << "? Renderer created\n";

    // =====================================================================
    // HUD PANEL SETUP (? NEW)
    // =====================================================================

    HUDPanel hudPanel(WIDTH, HEIGHT);
    gHUD = &hudPanel;
    hudPanel.setVisible(true);

    std::cout << "? HUD panel created\n";

    // =====================================================================
    // SHADER LOADING
    // =====================================================================

    ShaderGL* pipeShader = ShaderManager::instance().load(
        "pipe",
        "Source/ShaderFiles/pipe.vert",
        "Source/ShaderFiles/pipe.frag"
    );

    if (!pipeShader)
    {
        std::cerr << "? Failed to load pipe shader\n";
        return -1;
    }

    std::cout << "? Shaders loaded\n";

    // =====================================================================
    // SIMULATION CONTROLLER SETUP
    // =====================================================================

    SimulationController simulator;
    gSimulator = &simulator;

    // =====================================================================
    // LOAD PROGRAM
    // =====================================================================

    std::vector<Operation> program;

    // Create test program
    Operation feed1, bend1, feed2;

    feed1.type = Operation::FEED;
    feed1.length = 100.0;

    bend1.type = Operation::BEND;
    bend1.R = 50.0;
    bend1.angle = PI / 2.0;

    feed2.type = Operation::FEED;
    feed2.length = 100.0;

    program.push_back(feed1);
    program.push_back(bend1);
    program.push_back(feed2);

    simulator.loadProgram(program);

    std::cout << "? Program loaded (" << program.size() << " operations)\n";
    std::cout << "\n";

    // =====================================================================
    // INPUT CALLBACKS
    // =====================================================================

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    std::cout << "? Input callbacks registered\n";

    // =====================================================================
    // RENDER MODE
    // =====================================================================

    RenderMode mode = RenderMode::MESH;

    std::cout << "? Render mode: MESH\n";
    std::cout << "\n";
    std::cout << "???????????????????????????????????????????????????????????????\n";
    std::cout << "SIMULATION READY - Press [P] to start\n";
    std::cout << "???????????????????????????????????????????????????????????????\n\n";

    // =====================================================================
    // FRAME TIMING INITIALIZATION
    // =====================================================================

    lastFrameTime = std::chrono::high_resolution_clock::now();

    // =====================================================================
    // MAIN RENDER LOOP
    // =====================================================================

    while (!glfwWindowShouldClose(window))
    {
        // =================================================================
        // FRAME TIMING
        // =================================================================

        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<double>(
            currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        if (deltaTime > 0.05) deltaTime = 0.05;

        // =================================================================
        // PHASE 1: INPUT PROCESSING
        // =================================================================

        processKeyboardInput(window, mode);

        // =================================================================
        // PHASE 2: SIMULATION UPDATE
        // =================================================================

        simulator.update(deltaTime);

        // =================================================================
        // PHASE 3: GEOMETRY & MESH GENERATION
        // =================================================================

        const PipeAxis3D& pipeGeometry = simulator.getPipeGeometry();
        const auto& nodes = pipeGeometry.getNodes();

        std::vector<Vec3D> points;
        std::vector<Vec3D> tangents;

        for (const auto& node : nodes)
        {
            points.push_back(node.pos);
            tangents.push_back(node.T);
        }

        if (!points.empty())
        {
            mesh.generate(points, tangents, radius, segments);
            renderer.uploadMesh(mesh.getVertices(), mesh.getIndices());
        }

        // =================================================================
        // PHASE 4: UPDATE HUD DATA (? NEW)
        // =================================================================

        hudPanel.update(simulator, mode);

        // =================================================================
        // PHASE 5: RENDERING - 3D SCENE
        // =================================================================

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.setMode(mode);
        pipeShader->use();

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjection((float)WIDTH, (float)HEIGHT);
        glm::mat4 MVP = projection * view * model;

        pipeShader->setMat4("MVP", MVP);
        pipeShader->setMat4("Model", model);
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        pipeShader->setMat3("NormalMatrix", normalMatrix);

        pipeShader->setVec3("lightPos", glm::vec3(100.0f, 100.0f, 100.0f));
        pipeShader->setVec3("viewPos", camera.getPosition());
        pipeShader->setVec3("objectColor", glm::vec3(0.2f, 0.8f, 0.3f));

        renderer.draw();

        // ===== HUD PASS (FIXED) =====
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        hudPanel.render();

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        // =================================================================
        // PHASE 6: RENDERING - 2D HUD OVERLAY (? NEW)
        // =================================================================

        

        // =================================================================
        // PHASE 7: SWAP BUFFERS & POLL EVENTS
        // =================================================================

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // =====================================================================
    // CLEANUP
    // =====================================================================

    std::cout << "\n\n";
    std::cout << "?????????????????????????????????????????????????????????????????\n";
    std::cout << "?                     SIMULATION COMPLETE                       ?\n";
    std::cout << "?               Thank you for using Phase 4B!                   ?\n";
    std::cout << "?????????????????????????????????????????????????????????????????\n";

    glfwTerminate();
    return 0;
}