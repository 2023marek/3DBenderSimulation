#pragma once

#include <glad/gl.h>
#include <QOpenGLWidget>
class AppController;
#include "Render/HUDPanel.h"  
#include "Render/ShaderGL.h"
#include "Render/ControlCamera.h"
#include "Render/TubeMesh.h"
#include "Render/PipeRenderer.h"
#include "Core/Geometry/PipeNode.h"
#include "Render/MachineRenderer.h"
#include "Core/Geometry/Frame.h"
#include "Core/Manufacturing/ManufacturingTypes.h"


class GLView : public QOpenGLWidget
{
    Q_OBJECT

public:
    GLView();
  
   
    
protected:
    void initializeGL() override;
    void paintGL() override;
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    ControlCamera camera;
    QPoint lastMousePos;
    bool leftPressed = false;
    bool rightPressed = false;
    bool autoFrame = true;   // run once
    bool showMachineReference = true; //machine reference
    glm::vec3 computePipeCenterAndSize(float& outSize);
    void drawDebugPoint(
        const Vec3D& p,
        double size);
    void drawDebugFrame(
        const Frame& frame,
        double size);

    // =========================
    // DATA SOURCE
    // =========================
   
    // =========================
    // SHADER
    // =========================
    ShaderGL* shader = nullptr;

  

    // =========================
    // PIPE RENDERING (NEW)
    // =========================
   
    int pipeVertexCount = 0;

    void uploadPipeGeometry();   // 
private:
    // =====================================================
    // OWNER:
    // GLView owns conversion from PipeAxis3D render-zone
    // nodes into TubeMesh input arrays.
    //
    // ACCESS:
    // private
    //
    // Reason:
    // Only GLView needs these helpers during rendering.
    // =====================================================
    void uploadCadPipeGeometry();

    void nodesToCenterlineAndTangents(
        const std::vector<PipeNode>& nodes,
        std::vector<Vec3D>& centers,
        std::vector<Vec3D>& tangents);

    void drawTubeZone(
        const std::vector<PipeNode>& nodes,
        double radius,
        int radialSegments);
    void drawMachineReference();

public:
    
    void setHUDData(const HUDData& data)
    {
        hudData = data;
    }

public:
    void setAppController(AppController* a)
    {
        app = a;
    }

//protected:
    //void keyPressEvent(QKeyEvent* event) override;

private:
    AppController* app = nullptr;
    HUDPanel* hud = nullptr;
    HUDData hudData;
    RenderMode renderMode = RenderMode::LINE;
    MachineRenderer machineRenderer;
public:
    void setRenderMode(RenderMode mode)
    {
        renderMode = mode;
    }
private:
    PipeRenderer pipeRenderer;

    TubeMesh tubeMesh;
    void drawManufacturingMeshZones(
        const ManufacturingRenderData& data
    );

    //GLuint meshVAO = 0 ;
   // GLuint meshVBO = 0;
    //GLuint meshEBO = 0;

    int meshIndexCount = 0;

    void drawColoredManufacturingTubeZone(
        const std::vector<PipeNode>& nodes,
        const glm::vec3& color
    );

    void drawCadPreview();

    void drawPlannedShapePreview();

    void drawManufacturingPlayback();
};