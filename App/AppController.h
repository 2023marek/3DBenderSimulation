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
#include "Core/Forming/PassPlacement.h"
#include "Core/Forming/ManufacturingHistory.h"
#include "Core/Forming/DeformableRegionSelection.h"
#include "Core/Forming/LocalDeformableRegion.h"
#include "Core/Geometry/SpatialCurveAccuracyReport.h"
#include "Core/Geometry/SpatialCurveIntegrationResult.h"
#include "Core/Forming/StretchBendingProcessInput.h"
#include "Core/Geometry/CurvatureTorsionProfile.h"


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
    void togglePlannedPreviewDebug();
    void toggleExplicitAttachMode();
	//Getters/setters

    
    void setShowInsertionMarker(bool enabled)
    {
        showInsertionMarker = enabled;
    }

    void setShowInsertionFrame(bool enabled)
    {
        showInsertionFrame = enabled;
    }

    void setShowTransformedInsertOverlay(bool enabled)
    {
        showTransformedInsertOverlay = enabled;
    }

    bool shouldShowInsertionMarker() const
    {
        return showInsertionMarker;
    }

    bool shouldShowInsertionFrame() const
    {
        return showInsertionFrame;
    }

    bool shouldShowTransformedInsertOverlay() const
    {
        return showTransformedInsertOverlay;
    }

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
    
     void togglePlacementPreset();


     //Getter

     const DeformableRegionSelection&
         getLastDeformableRegionSelection() const;

     bool isDeformableRegionOverlayVisible() const
     {
         return deformableRegionOverlayVisible;
     }

     void toggleDeformableRegionOverlay()
     {
         deformableRegionOverlayVisible =
             !deformableRegionOverlayVisible;
     }

     //Getter
     const LocalDeformableRegion&
         getLastLocalDeformableRegion() const;

     //Getter
     const SpatialCurveIntegrationResult&
         getDebugPlanarIntegrationResult() const
     {
         return debugPlanarIntegrationResult;
     }

     const SpatialCurveIntegrationResult&
         getDebugHelixIntegrationResult() const
     {
         return debugHelixIntegrationResult;
     }

     bool isSpatialIntegratorPreviewVisible() const
     {
         return spatialIntegratorPreviewVisible;
     }

     void toggleSpatialIntegratorPreview()
     {
         spatialIntegratorPreviewVisible =
             !spatialIntegratorPreviewVisible;
     }
     const CurvatureTorsionProfile&
         getDebugStretchBendingProfile() const
     {
         return debugStretchBendingProfile;
     }
     // Getter
     const SpatialCurveIntegrationResult&
         getDebugStretchBendingIntegrationResult() const
     {
         return debugStretchBendingIntegrationResult;
     }

private:
    

    RenderMode renderMode = RenderMode::LINE;
    SimulationController sim;
    std::vector<Operation> buildTestOperations() const;

    ManufacturingPlan buildTestManufacturingPlan(
        const std::vector<Operation>& ops) const;

    void configureInitialMode();
    void configureManufacturingDebug();
    bool showInsertionMarker = true;
    bool showInsertionFrame = true;
    bool showTransformedInsertOverlay = false;
    bool plannedPreviewDebugVisible = true;
    bool deformableRegionOverlayVisible =        true;
    void configureControllerDebug();
    enum class TestPlacementPreset
    {
        ArcLength,
        NodeIndex,
        ExplicitFrame
    };

    PassPlacement buildTestPlacement(
        TestPlacementPreset preset) const;

    TestPlacementPreset activePlacementPreset =
        TestPlacementPreset::NodeIndex;

    std::vector<Operation> testOperations;
  
    void rebuildTestManufacturingPlan();
    //helper
    const char* testPlacementPresetToString(
        TestPlacementPreset preset) const;
    const char* activeAttachModeName() const;
    
   ExplicitFrameAttachMode activeExplicitAttachMode =
        ExplicitFrameAttachMode::InsertedOnly;

   void debugTestSpatialCurveIntegrator();
   void debugTestSpatialHelixIntegrator() ;


   //storing results
  
   SpatialCurveIntegrationResult
       debugPlanarIntegrationResult;

   SpatialCurveIntegrationResult
       debugHelixIntegrationResult;

   bool spatialIntegratorPreviewVisible =
       true;
  // Strech bending
   StretchBendingProcessInput
       buildTestStretchBendingProcessInput() const;

   void debugTestStretchBendingEvaluation() const;

   void debugTestStretchBendingFeasibilityCases() const;
   CurvatureTorsionProfile
       debugStretchBendingProfile;
   void debugTestStretchBendingProfileBuilder();
   SpatialCurveIntegrationResult
       debugStretchBendingIntegrationResult;
   void debugTestStretchBendingGeometry();
}; 
      