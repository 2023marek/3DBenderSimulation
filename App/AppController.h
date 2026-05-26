#pragma once
#include "Common/UserAction.h"
#include "Render/RenderMode.h"
#include "Core/SimulationController.h"
#include "Render/HUDData.h"   

class AppController
{
public:
    AppController();

    void update(double dt);
    // ===== RENDER MODE API =====
    RenderMode getRenderMode() const { return renderMode; }

    void toggleRenderMode()
    {
        renderMode = (renderMode == RenderMode::LINE)
            ? RenderMode::MESH
            : RenderMode::LINE;

        std::cout << "[MODE] "
            << (renderMode == RenderMode::LINE ? "LINE" : "MESH")
            << std::endl;
    }

    const PipeAxis3D& getPipeGeometry() const;
	// ===== SIMULATION MODE CADPreview vs ManufacturingPlayback API =====

    void setSimulationMode(
        SimulationController::SimulationMode newMode)
    {
        sim.setMode(newMode);
    }

    SimulationController::SimulationMode getSimulationMode() const
    {
        return sim.getMode();
    }

    // NEW
    HUDData buildHUDData() const;
public:
    void play() { sim.play(); }
    void pause() { sim.pause(); }
    void reset() { sim.reset(); }
    void step() { sim.step(); }
   
    void handleAction(UserAction action);
   

    GeometricPipeModel& getCadPipeGeometry()
    {
        return sim.getCadPipeGeometry();
    }

    const GeometricPipeModel& getCadPipeGeometry() const
    {
        return sim.getCadPipeGeometry();
    }

    
private:
	RenderMode renderMode = RenderMode::LINE;
    SimulationController sim;



};

      