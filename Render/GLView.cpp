#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include "Common/UserAction.h"
#include "Render/ControlCamera.h"
#include "GLView.h"
#include "App/AppController.h"
#include "Core/Manufacturing/ManufacturingPipeSimulator.h"
#include "Core/Machine/MachineRenderData.h"


#include "Core/BendDirection.h"
//#include "Render/ShaderGL.h"
#include "Core/Geometry/PipeNode.h"
#include <QOpenGLContext>
#include <iostream>

GLView::GLView()
{
    setFocusPolicy(Qt::StrongFocus);
    camera.pitch = 20.0f;
    camera.yaw = -45.0f;
}

static std::vector<float> nodesToFloatLine(
    const std::vector<PipeNode>& nodes)
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


// =====================================================
// SHARED MANUFACTURING ZONE ORDER
//
// This order is used by both LINE and MESH rendering:
//
// 1. incoming stock
// 2. positioned straight
// 3. frozen geometry
// 4. current bend trace
// 5. active zone last
//
// Active zone is drawn last because it represents the
// current deformation/process focus.
// =====================================================

namespace
{
    std::vector<std::vector<float>> buildManufacturingLineStrips(
        const ManufacturingRenderData& data
    )
    {
        std::vector<std::vector<float>> strips;

        strips.push_back(nodesToFloatLine(data.incomingStockNodes));
        strips.push_back(nodesToFloatLine(data.positionedStraightNodes));
        strips.push_back(nodesToFloatLine(data.frozenNodes));
        strips.push_back(nodesToFloatLine(data.currentBendTraceNodes));
        strips.push_back(nodesToFloatLine(data.activeZoneNodes));

        return strips;
    }


    constexpr double MANUFACTURING_PIPE_RADIUS =
        5.0;

    constexpr int MANUFACTURING_PIPE_RADIAL_SEGMENTS =
        12;

    constexpr float MANUFACTURING_LINE_WIDTH =
        2.0f;


    // cad constns
    constexpr float CAD_LINE_WIDTH =
        2.0f;

    constexpr double CAD_PIPE_RADIUS =
        5.0;

    constexpr int CAD_PIPE_RADIAL_SEGMENTS =
        12;

    constexpr float PLAN_PREVIEW_LINE_WIDTH =
        2.0f;

    constexpr double PLAN_PREVIEW_PIPE_RADIUS =
        5.0;

    constexpr int PLAN_PREVIEW_PIPE_RADIAL_SEGMENTS =
        12;


    //===
    const glm::vec3 MANUFACTURING_INCOMING_COLOR =
        glm::vec3(0.45f, 0.45f, 0.45f);

    const glm::vec3 MANUFACTURING_POSITIONED_COLOR =
        glm::vec3(0.9f, 0.8f, 0.2f);

    const glm::vec3 MANUFACTURING_FROZEN_COLOR =
        glm::vec3(0.2f, 0.9f, 0.3f);

    const glm::vec3 MANUFACTURING_TRACE_COLOR =
        glm::vec3(0.2f, 0.7f, 1.0f);

    const glm::vec3 MANUFACTURING_ACTIVE_COLOR =
        glm::vec3(1.0f, 0.25f, 0.15f);

    // =====================================================
// Default pipe rendering colors
// =====================================================

    const glm::vec3 DEFAULT_PIPE_COLOR =
        glm::vec3(0.9f, 0.9f, 0.3f);

    constexpr glm::vec3 DEFORMABLE_REGION_OVERLAY_COLOR =
        glm::vec3(
            1.0f,
            0.2f,
            0.8f
        );

//helix preview

    constexpr glm::vec3 HELIX_PREVIEW_COLOR =
        glm::vec3(
            0.2f,
            1.0f,
            0.4f
        );

    constexpr float HELIX_PREVIEW_LINE_WIDTH =
        5.0f;

    constexpr double HELIX_PREVIEW_PIPE_RADIUS =
        2.5;


    //
    constexpr float DEFORMABLE_REGION_LINE_WIDTH =
        6.0f;

    constexpr double DEFORMABLE_REGION_PIPE_RADIUS =
        5.8;

const glm::vec3 CAD_PIPE_COLOR =
DEFAULT_PIPE_COLOR;

const glm::vec3 PLAN_PREVIEW_PIPE_COLOR =
DEFAULT_PIPE_COLOR;


const glm::vec3 PLAN_PREVIEW_MARKER_COLOR =
glm::vec3(1.0f, 0.2f, 0.2f);

const glm::vec3 PLAN_PREVIEW_FRAME_COLOR =
glm::vec3(1.0f, 0.8f, 0.1f);


constexpr double PLAN_PREVIEW_MARKER_SIZE =
8.0;

constexpr double PLAN_PREVIEW_FRAME_SIZE =
8.0;




}







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

static std::vector<float> pointsToFloatLine(
    const std::vector<Vec3D>& points)
{
    std::vector<float> data;
    data.reserve(points.size() * 3);

    for (const auto& p : points)
    {
        data.push_back(static_cast<float>(p.x));
        data.push_back(static_cast<float>(p.y));
        data.push_back(static_cast<float>(p.z));
    }

    return data;
}

// =====================================================
// SHARED MANUFACTURING ZONE ORDER
//
// This order is used by both LINE and MESH rendering:
//
// 1. incoming stock
// 2. positioned straight
// 3. frozen geometry
// 4. current bend trace
// 5. active zone last
//
// Active zone is drawn last because it represents the
// current deformation/process focus.
// =====================================================


void GLView::drawManufacturingMeshZones(
    const ManufacturingRenderData& data
)
{
    // Manufacturing zone draw order:
    // 1. incoming stock
    // 2. positioned straight
    // 3. frozen geometry
    // 4. current bend trace
    // 5. active zone last

    drawColoredManufacturingTubeZone(
        data.incomingStockNodes,
        MANUFACTURING_INCOMING_COLOR
    );

    drawColoredManufacturingTubeZone(
        data.positionedStraightNodes,
        MANUFACTURING_POSITIONED_COLOR
    );

    drawColoredManufacturingTubeZone(
        data.frozenNodes,
        MANUFACTURING_FROZEN_COLOR
    );

    drawColoredManufacturingTubeZone(
        data.currentBendTraceNodes,
        MANUFACTURING_TRACE_COLOR
    );

    drawColoredManufacturingTubeZone(
        data.activeZoneNodes,
        MANUFACTURING_ACTIVE_COLOR
    );

    
}



void GLView::drawDebugPoint(
    const Vec3D& p,
    double size)
{
    std::vector<std::vector<float>> strips;

    strips.push_back(
        pointsToFloatLine({
            { p.x - size, p.y, p.z },
            { p.x + size, p.y, p.z }
            })
    );

    strips.push_back(
        pointsToFloatLine({
            { p.x, p.y - size, p.z },
            { p.x, p.y + size, p.z }
            })
    );

    strips.push_back(
        pointsToFloatLine({
            { p.x, p.y, p.z - size },
            { p.x, p.y, p.z + size }
            })
    );

    pipeRenderer.setMode(RenderMode::LINE);
    pipeRenderer.uploadLineStrips(strips);

    glLineWidth(4.0f);
    pipeRenderer.draw();
}



void GLView::drawDebugFrame(
    const Frame& frame,
    double size)
{
    // =====================================================
    // DEBUG FRAME DRAWING
    //
    // Draws local frame axes at a selected point.
    //
    // T axis: tangent direction
    // N axis: normal direction
    // B axis: binormal direction
    //
    // Current renderer uses one color for all strips.
    // Later we can draw each axis with separate color.
    // =====================================================

    Vec3D P =
        frame.P;

    Vec3D T =
        frame.T.normalized();

    Vec3D N =
        frame.N.normalized();

    Vec3D B =
        frame.B.normalized();

    std::vector<std::vector<float>> strips;

    // Tangent axis
    strips.push_back(
        pointsToFloatLine({
            P,
            P + T * size
            })
    );

    // Normal axis
    strips.push_back(
        pointsToFloatLine({
            P,
            P + N * size
            })
    );

    // Binormal axis
    strips.push_back(
        pointsToFloatLine({
            P,
            P + B * size
            })
    );

    pipeRenderer.setMode(
        RenderMode::LINE
    );

    pipeRenderer.uploadLineStrips(
        strips
    );

    glLineWidth(
        4.0f
    );

    pipeRenderer.draw();
}

// =========================
// Helper for machine reference (axes)
// 
//static std::vector<float> pointsToFloatLine(
 //   const std::vector<Vec3D>& points)
//{
 //   std::vector<float> data;
  //  data.reserve(points.size() * 3);

  //  for (const auto& p : points)
  //  {
  //      data.push_back(static_cast<float>(p.x));
  //      data.push_back(static_cast<float>(p.y));
  //      data.push_back(static_cast<float>(p.z));
   // }

  //  return data;
//}
//H 






//===================================
// 

// =========================
// CONNECT PIPE DATA
// =========================



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
    machineRenderer.init();
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
    const std::vector<PipeNode>& nodes,
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
    const std::vector<PipeNode>& nodes,
    double radius,
    int radialSegments)
{
    
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

void GLView::drawMachineReference()
{
    if (!app)
        return;

    MachineRenderData data =
        app->getMachineRenderData();

    // =====================================================
    // Draw imported STL machine parts
    // =====================================================

    shader->setVec3(
        "pipeColor",
        glm::vec3(0.45f, 0.45f, 0.48f)
    );

    machineRenderer.drawParts(data);

    // =====================================================
    // Draw simple reference overlay
    // =====================================================

    shader->setVec3(
        "pipeColor",
        glm::vec3(0.8f, 0.8f, 0.8f)
    );

    glLineWidth(1.5f);

    machineRenderer.drawReference(data);

    // Restore pipe color for next frame safety.
    shader->setVec3(
        "pipeColor",
        glm::vec3(0.2f, 0.9f, 0.3f)
    );
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

    //paintGL read like orchestration only :

    //setup frame
    //    setup camera
    //    setup shader
    //    dispatch render mode
    //    draw machine / HUD
//=============================================================================
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!shader)
        return;

    // =====================================================
    // CAMERA AUTO-FRAME
    //
    // Runs once to place camera around pipe.
    // =====================================================

    if (app && autoFrame)
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
    shader->setVec3(
        "pipeColor",
        DEFAULT_PIPE_COLOR
    );// green
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

    if (!app)
        return;

    auto mode =
        app->getSimulationMode();
   // std::cout << "[GLView] mode="
    //    << (mode == SimulationController::SimulationMode::CADPreview
    //        ? "CADPreview"
    //        : "ManufacturingPlayback")
    //    << std::endl;
    // 
    // 
    // 
   // std::cout << "[GLView] mode=";

    if (mode == SimulationController::SimulationMode::CADPreview)
    {
  //      std::cout << "CADPreview";
    }
    else if (mode == SimulationController::SimulationMode::PlannedShapePreview)
    {
    //    std::cout << "PlannedShapePreview";
    }
    else if (mode == SimulationController::SimulationMode::ManufacturingPlayback)
    {
   //     std::cout << "ManufacturingPlayback";
    }
    else
    {
    //    std::cout << "Unknown";
    }

    std::cout << std::endl;

   
    // =====================================================
    // CAD PREVIEW
    // Uses GeometricPipeModel.
    // One ideal continuous pipe.
    // =====================================================


    if (mode == SimulationController::SimulationMode::CADPreview)
    {
        drawCadPreview();
    }
    else if (mode == SimulationController::SimulationMode::PlannedShapePreview)
    {
        drawPlannedShapePreview();

    }

    else if (mode == SimulationController::SimulationMode::ManufacturingPlayback)
    {
        drawManufacturingPlayback();
    }





    if (showMachineReference)
    {
        drawMachineReference();
    }
    if (hud)
    {
        hud->update(hudData, RenderMode::LINE);
        hud->render();
    }
}




void GLView::uploadPipeGeometry()
{
    if (!app)
        return;

    const ManufacturingPipeSimulator& mfgPipe =
        app->getManufacturingPipe();

    const auto& data =
        mfgPipe.getManufacturingRenderData();

    pipeRenderer.uploadLineStrips(
        buildManufacturingLineStrips(data)
    );
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
    if (!app)
    {
        outSize = 100.0f;
        return glm::vec3(0.0f);
    }

    std::vector<PipeNode> combinedNodes;

    auto mode =
        app->getSimulationMode();

    if (mode == SimulationController::SimulationMode::CADPreview)
    {
        const auto& cadNodes =
            app->getCadPipeGeometry().getNodes();

        combinedNodes.insert(
            combinedNodes.end(),
            cadNodes.begin(),
            cadNodes.end()
        );
    }

    // Manufacturing zone draw order:
// 1. incoming stock
// 2. positioned straight
// 3. frozen geometry
// 4. current bend trace
// 5. active zone last
//
// Keep LINE and MESH paths in the same order.
    else if (mode == SimulationController::SimulationMode::ManufacturingPlayback)
    {
        const auto& data =
            app->getManufacturingPipe().getManufacturingRenderData();

        auto append =
            [&combinedNodes](const std::vector<PipeNode>& nodes)
            {
                combinedNodes.insert(
                    combinedNodes.end(),
                    nodes.begin(),
                    nodes.end()
                );
            };

        append(data.incomingStockNodes);
        append(data.positionedStraightNodes);
            append(data.frozenNodes);
        append(data.currentBendTraceNodes);
        append(data.activeZoneNodes);
    }

    if (combinedNodes.empty())
    {
        outSize = 100.0f;
        return glm::vec3(0.0f);
    }

    glm::vec3 minP(1e9f);
    glm::vec3 maxP(-1e9f);

    for (const auto& n : combinedNodes)
    {
        glm::vec3 p(
            static_cast<float>(n.pos.x),
            static_cast<float>(n.pos.y),
            static_cast<float>(n.pos.z)
        );

        minP = glm::min(minP, p);
        maxP = glm::max(maxP, p);
    }

    glm::vec3 center =
        (minP + maxP) * 0.5f;

    glm::vec3 sizeVec =
        maxP - minP;

    outSize =
        glm::length(sizeVec);

    if (outSize < 1.0f)
        outSize = 100.0f;

    return center;
}

void GLView::drawColoredManufacturingTubeZone(
    const std::vector<PipeNode>& nodes,
    const glm::vec3& color
)
{
    shader->setVec3(
        "pipeColor",
        color
    );

    drawTubeZone(
        nodes,
        MANUFACTURING_PIPE_RADIUS,
        MANUFACTURING_PIPE_RADIAL_SEGMENTS
    );
}

void GLView::uploadCadPipeGeometry()
{
    if (!app)
        return;

    auto& cadPipe =
        app->getCadPipeGeometry();

    const auto& cadNodes =
        cadPipe.getNodes();

    std::vector<float> line;

    line.reserve(cadNodes.size() * 3);

    for (const auto& node : cadNodes)
    {
        line.push_back(static_cast<float>(node.pos.x));
        line.push_back(static_cast<float>(node.pos.y));
        line.push_back(static_cast<float>(node.pos.z));
    }

    pipeRenderer.uploadLine(line);


}

void GLView::drawCadPreview()
{
    drawCadPreviewPipe();
}

void GLView::drawPlannedShapePreview()
{
    const auto& preview =
        app->getManufacturingPlanPreview();

    drawPlanPreviewPipe(
        preview
    );

    drawPlanPreviewInsertionMarker(
        preview
    );

    drawPlanPreviewInsertionFrame(
        preview
    );
}


void GLView::drawManufacturingPlayback()
{
    const ManufacturingPipeSimulator& mfgPipe =
        app->getManufacturingPipe();

    const ManufacturingRenderData& data =
        mfgPipe.getManufacturingRenderData();

    drawManufacturingPlaybackPipe(
        data
    );
}

void GLView::drawPlanPreviewInsertionMarker(
    const ManufacturingPlanPreviewModel& preview
)
{
    if (!preview.shouldShowInsertionMarker()
        || !preview.hasInsertionMarkerNode())
    {
        return;
    }

    const PipeNode& marker =
        preview.getInsertionMarkerNode();

    shader->setVec3(
        "pipeColor",
        PLAN_PREVIEW_MARKER_COLOR
    );

    drawDebugPoint(marker.pos, PLAN_PREVIEW_MARKER_SIZE);

    shader->setVec3(
        "pipeColor",
        PLAN_PREVIEW_PIPE_COLOR
    );
}

void GLView::drawPlanPreviewInsertionFrame(
    const ManufacturingPlanPreviewModel& preview
)
{
    if (!preview.shouldShowInsertionFrame()
       || !preview.hasInsertionStartFrame())
    {
        return;
    }

    const Frame& insertionFrame =
        preview.getInsertionStartFrame();

    shader->setVec3(
        "pipeColor",
        PLAN_PREVIEW_FRAME_COLOR
    );

    drawDebugFrame(insertionFrame, PLAN_PREVIEW_FRAME_SIZE);

    pipeRenderer.setMode(
        renderMode
    );

    shader->setVec3(
        "pipeColor",
        PLAN_PREVIEW_PIPE_COLOR
    );
}





void GLView::drawPlanPreviewPipe(
    const ManufacturingPlanPreviewModel& preview
)
{
    const auto& previewNodes =
        preview.getNodes();

    const auto& previewStrips =
        preview.getPreviewNodeStrips();

    bool useStrips =
        !previewStrips.empty();

    pipeRenderer.setMode(
        renderMode
    );

    shader->setVec3(
        "pipeColor",
        PLAN_PREVIEW_PIPE_COLOR
    );

    if (renderMode == RenderMode::LINE)
    {
        if (useStrips)
        {
            std::vector<std::vector<float>> strips;

            for (const auto& strip : previewStrips)
            {
                strips.push_back(
                    nodesToFloatLine(
                        strip
                    )
                );
            }

            pipeRenderer.uploadLineStrips(
                strips
            );
        }
        else
        {
            pipeRenderer.uploadLine(
                nodesToFloatLine(
                    previewNodes
                )
            );
        }

        glLineWidth(
            PLAN_PREVIEW_LINE_WIDTH
        );

        pipeRenderer.draw();
    }
    else if (renderMode == RenderMode::MESH)
    {
        if (useStrips)
        {
            for (const auto& strip : previewStrips)
            {
                drawTubeZone(
                    strip,
                    PLAN_PREVIEW_PIPE_RADIUS,
                    PLAN_PREVIEW_PIPE_RADIAL_SEGMENTS
                );
            }
        }
        else
        {
            drawTubeZone(
                previewNodes,
                PLAN_PREVIEW_PIPE_RADIUS,
                PLAN_PREVIEW_PIPE_RADIAL_SEGMENTS
            );
        }
    }
}


void GLView::drawCadPreviewPipe()
{
    auto& cadPipe =
        app->getCadPipeGeometry();

    const auto& cadNodes =
        cadPipe.getNodes();

    pipeRenderer.setMode(
        renderMode
    );

    shader->setVec3(
        "pipeColor",
        CAD_PIPE_COLOR
    );

    if (renderMode == RenderMode::LINE)
    {
        uploadCadPipeGeometry();

        glLineWidth(
            CAD_LINE_WIDTH
        );

        pipeRenderer.draw();
    }
    else if (renderMode == RenderMode::MESH)
    {
        drawTubeZone(
            cadNodes,
            CAD_PIPE_RADIUS,
            CAD_PIPE_RADIAL_SEGMENTS
        );
    }
}

void GLView::drawManufacturingPlaybackPipe(
    const ManufacturingRenderData& data
)
{
    pipeRenderer.setMode(
        renderMode
    );

    if (renderMode == RenderMode::LINE)
    {
        uploadPipeGeometry();

        glLineWidth(
            MANUFACTURING_LINE_WIDTH
        );

        pipeRenderer.draw();
    }
    else if (renderMode == RenderMode::MESH)
    {
        drawManufacturingMeshZones(
            data
        );
    }

    if (!app)
        return;

    // Existing magenta source-region overlay.
    if (app->isDeformableRegionOverlayVisible())
    {
        drawDeformableRegionSelectionOverlay(
            app->getLastDeformableRegionSelection()
        );
    }

    // New green world-space helix preview.
    drawWorldHelixPreviewOverlay(
        app->getLastLocalDeformableRegion()
    );
}


void GLView::drawDeformableRegionSelectionOverlay(
    const DeformableRegionSelection& selection
)
{
    if (!selection.valid)
        return;

    if (selection.selectedNodes.size() < 2)
        return;

    shader->setVec3(
        "pipeColor",
        DEFORMABLE_REGION_OVERLAY_COLOR
    );

    if (renderMode == RenderMode::LINE)
    {
        pipeRenderer.setMode(
            RenderMode::LINE
        );

        pipeRenderer.uploadLine(
            nodesToFloatLine(
                selection.selectedNodes
            )
        );

        glLineWidth(
            DEFORMABLE_REGION_LINE_WIDTH
        );

        pipeRenderer.draw();

        // Overlay upload replaced the main line buffer.
        // Restore it for the next frame.
        uploadPipeGeometry();

        glLineWidth(
            MANUFACTURING_LINE_WIDTH
        );
    }
    else if (renderMode == RenderMode::MESH)
    {
        drawTubeZone(
            selection.selectedNodes,
            DEFORMABLE_REGION_PIPE_RADIUS,
            MANUFACTURING_PIPE_RADIAL_SEGMENTS
        );
    }

    pipeRenderer.setMode(
        renderMode
    );

    shader->setVec3(
        "pipeColor",
        DEFAULT_PIPE_COLOR
    );
}


void GLView::drawWorldHelixPreviewOverlay(
    const LocalDeformableRegion& region
)
{
    if (!region.valid)
        return;

    if (region.worldPreviewHelixNodes.size() < 2)
        return;

    shader->setVec3(
        "pipeColor",
        HELIX_PREVIEW_COLOR
    );

    if (renderMode == RenderMode::LINE)
    {
        pipeRenderer.setMode(
            RenderMode::LINE
        );

        pipeRenderer.uploadLine(
            nodesToFloatLine(
                region.worldPreviewHelixNodes
            )
        );

        glLineWidth(
            HELIX_PREVIEW_LINE_WIDTH
        );

        pipeRenderer.draw();

        // Restore the normal manufacturing line geometry,
        // because the shared renderer buffer was replaced.
        uploadPipeGeometry();

        glLineWidth(
            MANUFACTURING_LINE_WIDTH
        );
    }
    else if (renderMode == RenderMode::MESH)
    {
        drawTubeZone(
            region.worldPreviewHelixNodes,
            HELIX_PREVIEW_PIPE_RADIUS,
            MANUFACTURING_PIPE_RADIAL_SEGMENTS
        );
    }

    pipeRenderer.setMode(
        renderMode
    );

    shader->setVec3(
        "pipeColor",
        DEFAULT_PIPE_COLOR
    );
}







