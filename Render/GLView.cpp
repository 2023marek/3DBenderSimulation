#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include "Common/UserAction.h"
#include "Render/ControlCamera.h"
#include "GLView.h"
#include "Core/SimulationController.h"
#include "Render/ShaderGL.h"
#include "Core/PipeUtils.h"
#include <QOpenGLContext>
#include <iostream>

// =========================
// SIMPLE SHADERS (GPU PROGRAM)
// =========================
static const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(aPos, 1.0);
}
)";

static const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(0.2, 0.9, 0.3, 1.0); // green color
}
)";


//

// =========================
// INITIALIZE OPENGL (RUNS ONCE)
// =========================
void GLView::initializeGL()
{
    
    // Get current OpenGL context from Qt
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    std::cout << "[HUD] created\n";
    if (!ctx)
    {
        std::cout << "No OpenGL context!\n";
        return;
    }

    // =========================
    // LOAD GLAD (function pointers)
    // =========================
    auto loader = [](const char* name) -> void*
        {
            return reinterpret_cast<void*>(
                QOpenGLContext::currentContext()->getProcAddress(name)
                );
        };

    if (!gladLoadGLLoader((GLADloadproc)loader))
    {
        std::cout << "Failed to initialize GLAD\n";
        return;
    }
    pipeRenderer.init();
    // =========================
    // CREATE SHADER (YOUR CLASS)
    // =========================
    shader = new ShaderGL(vertexShaderSrc, fragmentShaderSrc);

    hud = new HUDPanel(width(), height());
    // =========================
// HUD SHADERS (ADD THIS BLOCK)
// =========================

// ---- RECT / PANEL SHADER ----
    const char* hudVertex = R"(
#version 330 core
layout(location = 0) in vec2 aPos;

uniform vec2 uScreenSize;
uniform vec4 uRect;

void main()
{
    vec2 pos = uRect.xy + aPos * uRect.zw;

    vec2 ndc = vec2(
        (pos.x / uScreenSize.x) * 2.0 - 1.0,
        1.0 - (pos.y / uScreenSize.y) * 2.0
    );

    gl_Position = vec4(ndc, 0.0, 1.0);
}
)";

    const char* hudFrag = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 uColor;

void main()
{
    FragColor = uColor;
}
)";

    // ---- TEXT SHADER ----
    const char* textVertex = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

out vec2 TexCoord;

uniform mat4 projection;

void main()
{
    TexCoord = aUV;
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
)";

    const char* textFrag = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D fontTex;
uniform vec4 textColor;

void main()
{
    float alpha = texture(fontTex, TexCoord).r;
    FragColor = vec4(textColor.rgb, alpha);
}
)";
    // 1. CREATE SHADERS FIRST
    ShaderGL* hudShader = new ShaderGL(hudVertex, hudFrag);
    ShaderGL* textShader = new ShaderGL(textVertex, textFrag);

    // 2. THEN CONNECT THEM TO HUD
    hud->setHUDShader(hudShader);
    hud->setTextShader(textShader);

    //glGenVertexArrays(1, &pipeVAO);
    //glGenBuffers(1, &pipeVBO);

    //glBindVertexArray(pipeVAO);
    //glBindBuffer(GL_ARRAY_BUFFER, pipeVBO);

    // no data yet!
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);


    // =========================
    // BASIC SETTINGS
    // =========================
    glEnable(GL_DEPTH_TEST);       // enable depth (3D)
    glClearColor(0, 0, 0, 1);      // black background
}


// =========================
// RENDER FRAME (RUNS EVERY FRAME)
// =========================



void GLView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    std::cout << "[HUD] render call\n";
    if (!shader) return;

    shader->use();
    uploadPipeGeometry();
    pipeRenderer.setMode(renderMode);
    pipeRenderer.draw();
    if (controller && autoFrame)
    {
        float size = 100.0f;
        glm::vec3 center = computePipeCenterAndSize(size);

        camera.target = center;
        camera.distance = size * 2.5f + 20.0f;
   
        camera.pitch = 25.0f;
        camera.yaw = -45.0f;
        autoFrame = false; // ?? only once
    }

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjection(width(), height());
    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 mvp = proj * view * model;

    shader->setMat4("MVP", mvp);

    if (pipeVertexCount > 1)
    {
        glLineWidth(2.0f);
       // glBindVertexArray(pipeVAO);
        //glDrawArrays(GL_LINE_STRIP, 0, pipeVertexCount);
    }
    // ===== DRAW HUD LAST =====
    if (hud)
    {
        hud->update(hudData, RenderMode::LINE);
        hud->render();
    }
}


void GLView::uploadPipeGeometry()
{
    if (!controller) return;

    const auto& nodes =
        controller->getPipeGeometry().getNodes();

    if (nodes.empty()) return;

    // ?? CLIP HERE (KEY!)
    double visibleLength = controller->getTotalFedLength();
    auto clippedNodes = clipByLength(nodes, visibleLength);

    // ===== LINE =====
    std::vector<float> lineData;
    lineData.reserve(clippedNodes.size() * 3);
    std::cout << "[DEBUG] using clipping pipeline\n";
    for (const auto& n : clippedNodes)
    {
        lineData.push_back(n.pos.x);
        lineData.push_back(n.pos.y);
        lineData.push_back(n.pos.z);
    }
    std::cout << "[CLIP] visibleLength: " << visibleLength << "\n";
    std::cout << "[CLIP] nodes full: " << nodes.size()
        << " clipped: " << clippedNodes.size() << "\n";
    pipeRenderer.uploadLine(lineData);

    // ===== MESH =====
    if (renderMode == RenderMode::MESH)
    {
        std::vector<Vec3D> C;
        std::vector<Vec3D> T;

        for (const auto& n : clippedNodes)
        {
            C.push_back(n.pos);
            T.push_back(n.T);
        }

        tubeMesh.generate(C, T, 5.0, 12);

        pipeRenderer.uploadMesh(
            tubeMesh.getVertices(),
            tubeMesh.getIndices()
        );
    }
}

void GLView::mousePressEvent(QMouseEvent* event)
{
    lastMousePos = event->pos();

    if (event->button() == Qt::LeftButton)
        leftPressed = true;

    if (event->button() == Qt::RightButton)
        rightPressed = true;
}

void GLView::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();

    if (leftPressed)
    {
        // ORBIT
        camera.processMouseMovement(delta.x(), -delta.y());
    }
    else if (rightPressed)
    {
        // PAN
        camera.processPan(delta.x(), -delta.y());
    }

    update(); // trigger repaint
}
void GLView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        leftPressed = false;

    if (event->button() == Qt::RightButton)
        rightPressed = false;
}

void GLView::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().y() / 120.0f;
    camera.processScroll(delta);

    update();
}



glm::vec3 GLView::computePipeCenterAndSize(float& outSize)
{
    const auto& nodes = controller->getPipeGeometry().getNodes();
    ;

    if (nodes.empty())
    {
        outSize = 100.0f;
        return glm::vec3(0.0f);
    }

    glm::vec3 minP(1e9f);
    glm::vec3 maxP(-1e9f);

    for (const auto& n : nodes)
    {
        glm::vec3 p(n.pos.x, n.pos.y, n.pos.z);

        minP = glm::min(minP, p);
        maxP = glm::max(maxP, p);
    }

    glm::vec3 center = (minP + maxP) * 0.5f;

    glm::vec3 sizeVec = maxP - minP;
    outSize = glm::length(sizeVec);

    return center;
}



