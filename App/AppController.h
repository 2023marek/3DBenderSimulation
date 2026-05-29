#pragma once

#include <iostream>

#include "Common/UserAction.h"
#include "Render/RenderMode.h"
#include "Core/SimulationController.h"
#include "Core/Manufacturing/ManufacturingPipeSimulator.h"
#include "Render/HUDData.h"

class AppController
{
public:
    AppController();

    void update(double dt);

    // =====================================================
    // RENDER MODE API
    // =====================================================

    RenderMode getRenderMode() const
    {
        return renderMode;
    }

    void toggleRenderMode()
    {
        renderMode =
            (renderMode == RenderMode::LINE)
            ? RenderMode::MESH
            : RenderMode::LINE;

        std::cout << "[MODE] "
            << (renderMode == RenderMode::LINE ? "LINE" : "MESH")
            << std::endl;
    }

    // =====================================================
    // MANUFACTURING PIPE API
    // =====================================================

    ManufacturingPipeSimulator& getManufacturingPipe()
    {
        return sim.getManufacturingPipe();
    }

    const ManufacturingPipeSimulator& getManufacturingPipe() const
    {
        return sim.getManufacturingPipe();
    }

    // =====================================================
    // LEGACY PIPE API
    // Still needed temporarily during refactor.
    // =====================================================

    

    // =====================================================
    // CAD PIPE API
    // =====================================================

    GeometricPipeModel& getCadPipeGeometry()
    {
        return sim.getCadPipeGeometry();
    }

    const GeometricPipeModel& getCadPipeGeometry() const
    {
        return sim.getCadPipeGeometry();
    }

    // =====================================================
    // SIMULATION MODE API
    // =====================================================

    void setSimulationMode(
        SimulationController::SimulationMode newMode)
    {
        sim.setMode(newMode);
    }

    SimulationController::SimulationMode getSimulationMode() const
    {
        return sim.getMode();
    }

    // =====================================================
    // PLAYBACK API
    // =====================================================

    void play()
    {
        sim.play();
    }

    void pause()
    {
        sim.pause();
    }

    void reset()
    {
        sim.reset();
    }

    void step()
    {
        sim.step();
    }

    void handleAction(UserAction action);

    HUDData buildHUDData() const;

private:
    RenderMode renderMode = RenderMode::LINE;
    SimulationController sim;
}; 
      