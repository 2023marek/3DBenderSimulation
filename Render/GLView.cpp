#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include "Common/UserAction.h"
#include "Render/ControlCamera.h"
#include "GLView.h"
#include "App/AppController.h"
//#include "Render/ShaderGL.h"
#include <QOpenGLContext>
#include <iostream>




// =========================
// SIMPLE SHADERS (GPU PROGRAM)
// =========================
static const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 MVP;
uniform mat4 model;

out vec3 Normal;
out vec3 FragPos;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = MVP * vec4(aPos, 1.0);
}
)";

static const char* fragmentShaderSrc = R"(
#version 330 core
in vec3 Normal;
in vec3 FragPos;
out vec4 FragColor;

uniform vec3 lightDir;   // Directional light (normalized)
uniform vec3 viewPos;    // Camera position
uniform vec3 pipeColor;  // Pipe base color

void main()
{
    // Ambient
    float ambientStrength = 0.8;
    vec3 ambient = ambientStrength * pipeColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diff * pipeColor;

    // Specular
    float specularStrength = 0.25;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)";
//HELPER
static std::vector<float> nodesToFloatLine(
    const std::vector<PipeAxis3D::Node>& nodes)
{
    std::vector<float> data;
    data.reserve(nodes.size() * 3);

    for (const auto& n : nodes)
    {
        data.push_back(static_cast<float>(n.pos.x));
        data.push_back(static_cast<float>(n.pos.y));
        data.push_back(static_cast<float>(n.pos.z));
    }

    return data;
}
// =========================
// CONNECT PIPE DATA
// =========================
void GLView::setPipe(const PipeAxis3D* p)
{
    pipe = p;
}


// =========================
// INITIALIZE OPENGL (RUNS ONCE)
// =========================
static void* qt_gl_get_proc(const char* name) {
    return reinterpret_cast<void*>(
        QOpenGLContext::currentContext()->getProcAddress(name)
        );
}

void GLView::initializeGL()
{
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    std::cout << "[HUD] created\n";
    if (!ctx)
    {
        std::cout << "No OpenGL context!\n";
        return;
    }

    // Cast the function pointer to the correct type
    if (!gladLoadGL((GLADloadfunc)&qt_gl_get_proc)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return;
    } 
   

   // glad_glEnable(GL_DEBUG_OUTPUT);
   // glad_glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    

    pipeRenderer.init();
    // =========================
    // CREATE SHADER (YOUR CLASS)
    // =========================
    shader = new ShaderGL(vertexShaderSrc, fragmentShaderSrc);
    
   
    hud = new HUDPanel(this->width(), this->height());
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
   // glad_glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

   // glad_glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
   // glad_glEnableVertexAttribArray(0);

   // glad_glBindVertexArray(0);


    // =========================
    // BASIC SETTINGS
    // =========================
    glad_glEnable(GL_DEPTH_TEST);       // enable depth (3D)
    glad_glClearColor(0, 0, 0, 1);      // black background
}
//=========================
//RENDER HELPER

void GLView::nodesToCenterlineAndTangents(
    const std::vector<PipeAxis3D::Node>& nodes,
    std::vector<Vec3D>& centers,
    std::vector<Vec3D>& tangents)
{
    // =====================================================
    // OWNER:
    // GLView owns render-data conversion.
    //
    // INPUT:
    // One manufacturing zone.
    //
    // OUTPUT:
    // Centerline + tangents for TubeMesh.
    //
    // IMPORTANT:
    // This converts ONE zone only.
    // It does not merge zones.
    // =====================================================

    centers.clear();
    tangents.clear();

    centers.reserve(nodes.size());
    tangents.reserve(nodes.size());

    for (const auto& n : nodes)
    {
        centers.push_back(n.pos);
        tangents.push_back(n.T);
    }
}


//Helper Draw TUBE ZONE

void GLView::drawTubeZone(
    const std::vector<PipeAxis3D::Node>& nodes,
    double radius,
    int radialSegments)
{
    // =====================================================
    // OWNER:
    // GLView owns per-zone mesh drawing.
    //
    // ACCESS:
    // private GLView helper.
    //
    // PIPEFLOW:
    //
    // one manufacturing zone
    //        ?
    // nodesToCenterlineAndTangents()
    //        ?
    // TubeMesh.generate()
    //        ?
    // PipeRenderer.uploadMesh()
    //        ?
    // PipeRenderer.draw()
    //
    // IMPORTANT:
    // Draw one zone at a time.
    // Never merge zones into one tube mesh.
    // =====================================================

    if (nodes.size() < 2)
        return;

    std::vector<Vec3D> centers;
    std::vector<Vec3D> tangents;

    nodesToCenterlineAndTangents(
        nodes,
        centers,
        tangents
    );

    tubeMesh.generate(
        centers,
        tangents,
        radius,
        radialSegments
    );

    pipeRenderer.uploadMesh(
        tubeMesh.getVertices(),
        tubeMesh.getIndices()
    );

    pipeRenderer.draw();
}
// =========================
// RENDER FRAME (RUNS EVERY FRAME)
// =========================




void GLView::paintGL()
{
    // =====================================================
    // OWNER:
    // GLView owns frame rendering orchestration.
    //
    // PipeAxis3D owns geometry data.
    // PipeRenderer owns OpenGL buffers/draw calls.
    // GLView decides WHICH render path to use.
    // =====================================================

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!shader)
        return;

    // =====================================================
    // CAMERA AUTO-FRAME
    //
    // Runs once to place camera around pipe.
    // =====================================================

    if (pipe && autoFrame)
    {
        float size = 100.0f;
        glm::vec3 center = computePipeCenterAndSize(size);

        camera.target = center;
        camera.distance = size * 2.5f + 20.0f;

        camera.pitch = 25.0f;
        camera.yaw = -45.0f;

        autoFrame = false;
    }

    // =====================================================
    // MVP MATRIX
    //
    // Must be set BEFORE drawing.
    // =====================================================

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjection(width(), height());
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = proj * view * model;

    shader->use();
    shader->setMat4("MVP", mvp);
    shader->setMat4("model", model);
    shader->setVec3("lightDir", glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f)));
    shader->setVec3("viewPos", camera.getPosition());
    shader->setVec3("pipeColor", glm::vec3(0.2f, 0.9f, 0.3f)); // green
    // =====================================================
    // MANUFACTURING RENDER PATH
    //
    // Four-zone manufacturing visualization:
    //
    // IncomingStock
    //      ?
    // PositionedStraight
    //      ?
    // CurrentBendTrace
    //      ?
    // FrozenGeometry
    //      ?
    // ActiveZone overlay
    //
    // IMPORTANT:
    // In MESH mode, DO NOT use flattened pipe->getNodes().
    // Each zone must be drawn as a separate tube.
    // =====================================================

    if (app &&
        app->getSimulationMode()
        == SimulationController::SimulationMode::ManufacturingPlayback &&
        pipe)
    {
        const auto& data =
            pipe->getManufacturingRenderData();

        pipeRenderer.setMode(renderMode);

        if (renderMode == RenderMode::LINE)
        {
            // =================================================
            // MANUFACTURING LINE MODE
            //
            // uploadPipeGeometry() should upload separate
            // GL_LINE_STRIP ranges using ManufacturingRenderData.
            // =================================================

            uploadPipeGeometry();

            glLineWidth(2.0f);

            pipeRenderer.draw();
        }

        else if (renderMode == RenderMode::MESH)
        {
            // =================================================
            // MANUFACTURING MESH MODE
            //
            // Draw one tube mesh per zone.
            //
            // This prevents tube faces from connecting:
            // old node ---------------> entryFrame
            // =================================================

            drawTubeZone(
                data.incomingStockNodes,
                5.0,
                12
            );

            drawTubeZone(
                data.positionedStraightNodes,
                5.0,
                12
            );

            drawTubeZone(
                data.currentBendTraceNodes,
                5.0,
                12
            );

            drawTubeZone(
                data.frozenNodes,
                5.0,
                12
            );

            drawTubeZone(
                data.activeZoneNodes,
                5.0,
                12
            );
        }
    }
    else
    {
        // =====================================================
        // CAD PREVIEW RENDER PATH
        //
        // CAD geometry is one continuous designed pipe.
        // Flattened getNodes() is correct here.
        // =====================================================

        uploadPipeGeometry();

        pipeRenderer.setMode(renderMode);

        glLineWidth(2.0f);

        pipeRenderer.draw();
    }

    // =====================================================
    // HUD
    //
    // Draw last so it appears above 3D scene.
    // =====================================================

    if (hud)
    {
        hud->update(hudData, RenderMode::LINE);
        hud->render();
    }
}
void GLView::uploadPipeGeometry()
{
    if (!pipe)
        return;

    // =====================================================
    // MANUFACTURING MODE
    //
    // Draw zones as separate line strips:
    //
    // Zone 1: Incoming stock
    // Zone 2: Positioned straight
    // Zone 3: Active bend zone
    // Zone 4: Frozen geometry
    //
    // This removes false connector lines.
    // =====================================================

    if (app &&
        app->getSimulationMode()
        == SimulationController::SimulationMode::ManufacturingPlayback)
    {
        const auto& data =
            pipe->getManufacturingRenderData();

        std::vector<std::vector<float>> strips;

        strips.push_back(
            nodesToFloatLine(data.incomingStockNodes)
        );

        strips.push_back(
            nodesToFloatLine(data.positionedStraightNodes)
        );

        strips.push_back(
            nodesToFloatLine(data.currentBendTraceNodes)
        );

        strips.push_back(
            nodesToFloatLine(data.frozenNodes)
        );

        strips.push_back(
            nodesToFloatLine(data.activeZoneNodes)
        );

        pipeRenderer.uploadLineStrips(strips);

        // For now, mesh mode still uses flattened getNodes().
        // We will improve multi-zone mesh later.
        if (renderMode == RenderMode::MESH)
        {
            const auto& nodes = pipe->getNodes();

            std::vector<Vec3D> C;
            std::vector<Vec3D> T;

            for (const auto& n : nodes)
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

        return;
    }

    // =====================================================
    // CAD PREVIEW MODE
    //
    // CAD pipe is one continuous designed centerline.
    // One line strip is correct here.
    // =====================================================

    const auto& nodes =
        pipe->getNodes();

    if (nodes.empty())
        return;

    pipeRenderer.uploadLine(
        nodesToFloatLine(nodes)
    );

    if (renderMode == RenderMode::MESH)
    {
        std::vector<Vec3D> C;
        std::vector<Vec3D> T;

        for (const auto& n : nodes)
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
    const auto& nodes = pipe->getNodes();

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






