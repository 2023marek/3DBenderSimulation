#pragma once

#include <glad/gl.h>
#include <QOpenGLWidget>
class AppController;
#include "Render/HUDPanel.h"  
#include "Core/PipeAxis3D.h"
#include "Render/ShaderGL.h"
#include "Render/ControlCamera.h"
#include "Render/TubeMesh.h"
#include "Render/PipeRenderer.h"




class GLView : public QOpenGLWidget
{
    Q_OBJECT

public:
    GLView() {
        setFocusPolicy(Qt::StrongFocus);
        camera.pitch = 20.0f;
        camera.yaw = -45.0f;
        setFocusPolicy(Qt::StrongFocus);
		
    }
  
    void setPipe(const PipeAxis3D* pipe);
    
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
    glm::vec3 computePipeCenterAndSize(float& outSize);

    // =========================
    // DATA SOURCE
    // =========================
    const PipeAxis3D* pipe = nullptr;

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

    void nodesToCenterlineAndTangents(
        const std::vector<PipeAxis3D::Node>& nodes,
        std::vector<Vec3D>& centers,
        std::vector<Vec3D>& tangents);

    void drawTubeZone(
        const std::vector<PipeAxis3D::Node>& nodes,
        double radius,
        int radialSegments);


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

public:
    void setRenderMode(RenderMode mode)
    {
        renderMode = mode;
    }
private:
    PipeRenderer pipeRenderer;

    TubeMesh tubeMesh;

    //GLuint meshVAO = 0 ;
   // GLuint meshVBO = 0;
    //GLuint meshEBO = 0;

    int meshIndexCount = 0;

    
};