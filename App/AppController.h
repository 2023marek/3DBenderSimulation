#pragma once

#include <iostream>
#include <vector>
#include "Common/UserAction.h"
#include "Render/RenderMode.h"
#include "Core/SimulationController.h"
#include "Core/Manufacturing/ManufacturingPipeSimulator.h"
#include "Render/HUDData.h"
#include "Core/Machine/MachineRuntimeState.h"
#include "Core/Machine/MachineRenderData.h"
#include "Core/Forming/ManufacturingPlanPreviewModel.h"
#include "Core/PipeSystem.h"
#include "Core/Operations.h"
#include "Core/Forming/ManufacturingPlan.h"

class AppController
{
public:
    AppController();

    void update(double dt);
    void useCADPreview();
    void usePlannedShapePreview();
    void useManufacturingPlayback(); 
    //helper
    void toggleSimulationMode();

    //ACCESSORS
    ManufacturingPlanPreviewModel& getManufacturingPlanPreview()
    {
        return sim.getManufacturingPlanPreview();
    }

    const ManufacturingPlanPreviewModel& getManufacturingPlanPreview() const
    {
        return sim.getManufacturingPlanPreview();
    }

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

    //Machine getters/setters
     MachineRuntimeState& getMachineRuntimeState()
    {
        return sim.getMachineSystem().getRuntimeState();
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

    MachineRenderData getMachineRenderData() const
    {
        return sim.getMachineRenderData();
    }

    void handleAction(UserAction action);

    HUDData buildHUDData() const;

   

private:
    RenderMode renderMode = RenderMode::LINE;
    SimulationController sim;
    std::vector<Operation> buildTestOperations() const;

    ManufacturingPlan buildTestManufacturingPlan(
        const std::vector<Operation>& ops) const;

    void configureInitialMode();
  
}; 
      