#include <sstream>
#include <iostream>
#include <vector>

#include "AppController.h"

#include "Core/Forming/ManufacturingPass.h"
#include "Core/Forming/ManufacturingPlan.h"
#include "Core/Forming/HelixOperation.h"
#include "Core/Forming/HelixFormingPassBuilder.h"
#include "Core/Forming/RotaryDrawPassBuilder.h"
#include "Core/Sampling/PipeCurveNodeQuery.h"
#include "Core/Geometry/Frame.h"
#include "Core/Forming/AdditionalFormingPass.h"
#include "Core/Forming/ManufacturingHistoryDebug.h"
#include "Core/Forming/ManufacturingHistoryBuilder.h"



//#include "Core/Sampling/PipeCurveSampler.h"

// =====================================
// CONSTRUCTOR
// =====================================

namespace
{
    constexpr double TEST_FEED_1_LENGTH = 198.0;
    constexpr double TEST_BEND_RADIUS = 20.0;
    constexpr double TEST_BEND_ANGLE = PI / 2.0;
    constexpr double TEST_FEED_2_LENGTH = 110.0;

    constexpr double TEST_INCOMING_STOCK_LENGTH = 300.0;

    constexpr double TEST_HELIX_LENGTH = 200.0;
    constexpr double TEST_HELIX_RADIUS = 10.0;
    constexpr double TEST_HELIX_PITCH = 15.0;
    constexpr double TEST_HELIX_FEED_SPEED = 40.0;

    constexpr double TEST_INSERT_ARC_LENGTH = 202.0;
    constexpr size_t TEST_INSERT_NODE_INDEX = 404;
    constexpr bool DEBUG_PRINT_MANUFACTURING_HISTORY =
        true;
    constexpr bool DEBUG_MANUFACTURING_SIMULATOR =
        true;

    constexpr bool DEBUG_MFG_ACTIVE_WINDOW =
        false;

    constexpr bool DEBUG_MFG_BEND_STEP =
        false;

    constexpr bool DEBUG_MFG_SNAPSHOT =
        true;
}



AppController::AppController()
{
    testOperations =
        buildTestOperations();

    sim.loadProgram(
        testOperations
    );

    sim.getManufacturingPipe().setIncomingStockLength(
        TEST_INCOMING_STOCK_LENGTH
    );


    configureManufacturingDebug();


    rebuildTestManufacturingPlan();

    configureInitialMode();

    std::cout << "[APP PLACEMENT PRESET] "
        << testPlacementPresetToString(
            activePlacementPreset
        )
        << std::endl;

    auto& preview =
        sim.getManufacturingPlanPreview();

    preview.setShowInsertionMarker(true);
    preview.setShowInsertionFrame(false);
    preview.setShowTransformedInsertOverlay(true);
}


void AppController::update(double dt)
{
    sim.update(dt);
}

HUDData AppController::buildHUDData() const
{
    HUDData data;

    data.isPlaying = sim.isPlaying();
    data.isPaused = sim.isPaused();
    data.speed = sim.getSpeed();

    const MachineRuntimeState& state = sim.getMachineRuntimeState();

    data.time = state.currentTime;
    data.feedPosition = state.feedPosition;
    data.rotationDeg = state.rotationAngle * 180.0 / PI;
    data.bendDeg = state.bendAngle * 180.0 / PI;

    data.feeding = state.feeding;
    data.rotating = state.rotating;
    data.bending = state.bending;
    

    data.machineStateName = "IDLE";

    if (state.feeding)
        data.machineStateName = "FEED";
    else if (state.rotating)
        data.machineStateName = "ROTATE";
    else if (state.bending)
        data.machineStateName = "BEND";

    if (data.isPlaying)
        data.status = "PLAYING";
    else if (data.isPaused)
        data.status = "PAUSED";
    else
        data.status = "IDLE";

    auto mode =
        sim.getMode();

    if (mode == SimulationController::SimulationMode::CADPreview)
    {
        data.simulationModeName =
            "CAD PREVIEW";
    }
    else if (mode == SimulationController::SimulationMode::PlannedShapePreview)
    {
        data.simulationModeName =
            "PLANNED SHAPE";
    }
    else if (mode == SimulationController::SimulationMode::ManufacturingPlayback)
    {
        data.simulationModeName =
            "MFG PLAYBACK";
    }
    else
    {
        data.simulationModeName =
            "UNKNOWN";
    }

    data.previewDebugName =
        plannedPreviewDebugVisible ? "ON" : "OFF";

    data.placementModeName =
      sim.getManufacturingPlanPreview().getActivePlacementModeName();
    data.attachModeName =
        activeAttachModeName();
    data.currentOpIndex = sim.getCurrentOperationIndex();
    data.totalOperations = sim.getTotalOperations();

    data.currentOpProgress = sim.getCurrentOperationProgress();
    data.overallProgress = sim.getOverallProgress();

    if (sim.getMode()
        == SimulationController::SimulationMode::ManufacturingPlayback)
    {
        data.nodeCount =
            sim.getManufacturingPipe().getNodes().size();
    }
    else if (sim.getMode()
        == SimulationController::SimulationMode::CADPreview)
    {
        data.nodeCount =
            sim.getCadPipeGeometry().getNodes().size();
    }
    else if (sim.getMode()
        == SimulationController::SimulationMode::PlannedShapePreview)
    {
        data.nodeCount =
            sim.getManufacturingPlanPreview()
            .getPreviewNodeCount();
    }
    else
    {
        data.nodeCount = 0;
    }





    std::ostringstream oss;

    const OperationQueue& queue = sim.getQueue();
    const Operation* currentOp = queue.getCurrent();

    if (currentOp)
    {
        if (currentOp->type == Operation::FEED)
        {
            oss << "FEED " << currentOp->length << " mm";
        }
        else if (currentOp->type == Operation::BEND)
        {
            double angleDeg = currentOp->angle * 180.0 / PI;
            oss << "BEND R=" << currentOp->R << " mm, angle=" << angleDeg << " deg";
        }
        else if (currentOp->type == Operation::ROTATE)
        {
            double angleDeg = currentOp->angle * 180.0 / PI;
            oss << "ROTATE " << angleDeg << " deg";
        }
        else
        {
            oss << "UNKNOWN OPERATION";
        }
    }
    else
    {
        oss << "NO OPERATION";
    }

    data.currentOpName = oss.str();

    return data;
}

void AppController::handleAction(UserAction action)
{
    switch (action)
    {
    case UserAction::Play:
        sim.play();
        break;

    case UserAction::Pause:
        sim.pause();
        break;

    case UserAction::Reset:
        sim.reset();
        break;

    case UserAction::Step:
        sim.step();
        break;

    case UserAction::ToggleRenderMode:
        toggleRenderMode();
        break;
    case UserAction::ToggleSimulationMode:
        toggleSimulationMode();
        break;

    case UserAction::TogglePlacementPreset:
        togglePlacementPreset();
        break;

    case UserAction::TogglePlannedPreviewDebug:
        togglePlannedPreviewDebug();
        break;

    case UserAction::ToggleExplicitAttachMode:
        toggleExplicitAttachMode();
        break;

    }
}

void AppController::useCADPreview()
{
    sim.setMode(
        SimulationController::SimulationMode::CADPreview
    );
   
}

void AppController::usePlannedShapePreview()
{
    sim.setMode(
        SimulationController::SimulationMode::PlannedShapePreview
    );
    
}

void AppController::useManufacturingPlayback()
{
    sim.setMode(
        SimulationController::SimulationMode::ManufacturingPlayback
    );
    
}

void AppController::toggleSimulationMode()
{
    auto mode =
        sim.getMode();

    if (mode == SimulationController::SimulationMode::CADPreview)
    {
        usePlannedShapePreview();
    }
    else if (mode == SimulationController::SimulationMode::PlannedShapePreview)
    {
        useManufacturingPlayback();
    }
    else if (mode == SimulationController::SimulationMode::ManufacturingPlayback)
    {
        useCADPreview();
    }
}



// Helpers
    std::vector<Operation> AppController::buildTestOperations() const
    {
        std::vector<Operation> ops;

        Operation op1;
        op1.type = Operation::FEED;
        op1.length = TEST_FEED_1_LENGTH;

        Operation op2;
        op2.type = Operation::BEND;
        op2.R = TEST_BEND_RADIUS;
        op2.angle = TEST_BEND_ANGLE;
        op2.bendDirection = BendDirection::CCW;

        Operation op3;
        op3.type = Operation::ROTATE;
        op3.angle = PI / 2.0;
        op3.rotationDirection = RotationDirection::CCW;

        Operation op4;
        op4.type = Operation::FEED;
        op4.length = TEST_FEED_2_LENGTH;

        ops.push_back(op1);
        ops.push_back(op2);
        ops.push_back(op3);
        ops.push_back(op4);

        return ops;
    }

    ManufacturingPlan AppController::buildTestManufacturingPlan(
        const std::vector<Operation>& ops) const
    {
        ManufacturingPass rotaryPass =
            RotaryDrawPassBuilder::buildPass(
                ops,
                "Rotary draw bending pass"
            );

        
      


        HelixOperation helixPassOp;

        helixPassOp.inputMode =
            HelixOperation::InputMode::RadiusPitch;

        helixPassOp.length = TEST_HELIX_LENGTH;
        helixPassOp.helixRadius = TEST_HELIX_RADIUS;
        helixPassOp.pitch = TEST_HELIX_PITCH;
        helixPassOp.feedSpeed = TEST_HELIX_FEED_SPEED;

        ManufacturingPass helixPass =
            HelixFormingPassBuilder::buildPass(
                helixPassOp,
                "Heating element helix pass"
            );

       helixPass.placement =
           buildTestPlacement(
               activePlacementPreset
           );
            

       
    
        ManufacturingPlan plan;

        plan.addPass(
            rotaryPass
        );

        plan.addPass(
            helixPass
        );

        return plan;




    }

    void AppController::configureInitialMode()
    {
        usePlannedShapePreview();

        // Alternatives for testing:
        //
        // useManufacturingPlayback();
        // useCADPreview();
    }

    PassPlacement AppController::buildTestPlacement(
        TestPlacementPreset preset) const
    {
        if (preset == TestPlacementPreset::ArcLength)
        {
            return PassPlacement::atArcLength(
                TEST_INSERT_ARC_LENGTH
            );
        }

        if (preset == TestPlacementPreset::NodeIndex)
        {
            return PassPlacement::atNodeIndex(
                TEST_INSERT_NODE_INDEX
            );
        }

        if (preset == TestPlacementPreset::ExplicitFrame)
        {
            Frame frame;

            frame.P = { 200.0, 80.0, 0.0 };
            frame.T = { 1.0, 0.0, 0.0 };
            frame.N = { 0.0, 1.0, 0.0 };
            frame.B = { 0.0, 0.0, 1.0 };

            return PassPlacement::atFrame(
                frame,
                activeExplicitAttachMode
            );
        }

        return PassPlacement::append();


        

    }


    void AppController::toggleExplicitAttachMode()
    {
        if (activeExplicitAttachMode
            == ExplicitFrameAttachMode::InsertedOnly)
        {
            activeExplicitAttachMode =
                ExplicitFrameAttachMode::AppendAfterFrame;
        }
        else if (activeExplicitAttachMode
            == ExplicitFrameAttachMode::AppendAfterFrame)
        {
            activeExplicitAttachMode =
                ExplicitFrameAttachMode::AttachBaseAfterInsert;
        }
        else if (activeExplicitAttachMode
            == ExplicitFrameAttachMode::AttachBaseAfterInsert)
        {
            activeExplicitAttachMode =
                ExplicitFrameAttachMode::InsertedOnly;
        }

        rebuildTestManufacturingPlan();

        std::cout << "[APP EXPLICIT ATTACH] "
            << explicitFrameAttachModeToString(
                activeExplicitAttachMode
            )
            << std::endl;
    }

    void AppController::togglePlacementPreset()
    {
        if (activePlacementPreset
            == TestPlacementPreset::ArcLength)
        {
            activePlacementPreset =
                TestPlacementPreset::NodeIndex;
        }
        else if (activePlacementPreset
            == TestPlacementPreset::NodeIndex)
        {
            activePlacementPreset =
                TestPlacementPreset::ExplicitFrame;
        }
        else if (activePlacementPreset
            == TestPlacementPreset::ExplicitFrame)
        {
            activePlacementPreset =
                TestPlacementPreset::ArcLength;
        }

        rebuildTestManufacturingPlan();

        std::cout << "[APP PLACEMENT PRESET] "
            << testPlacementPresetToString(
                activePlacementPreset
            )
            << std::endl;
    }

    void AppController::rebuildTestManufacturingPlan()
    {
        ManufacturingPlan plan =
            buildTestManufacturingPlan(
                testOperations
            );

        auto& preview =
            sim.getManufacturingPlanPreview();

        preview.setPlan(
            plan
        );

        ManufacturingHistory& history =
            sim.getManufacturingHistory();

        buildManufacturingHistoryFromPlan(
            plan,
            history
        );

        if (DEBUG_PRINT_MANUFACTURING_HISTORY)
        {
            std::cout << "[MFG HISTORY REAL PASSES] primary="
                << history.primaryPasses.size()
                << " additional="
                << history.additionalPasses.size()
                << std::endl;

            debugPrintManufacturingHistory(
                history
            );
        }

       


        preview.setDebugLogging(
            true
        );

        preview.setShowInsertionMarker(
            true
        );

        preview.setShowInsertionFrame(
            true
        );

        preview.setShowTransformedInsertOverlay(
            false
        );
    }

    const char* AppController::testPlacementPresetToString(
        TestPlacementPreset preset) const
    {
        if (preset == TestPlacementPreset::ArcLength)
        {
            return "ArcLength";
        }

        if (preset == TestPlacementPreset::NodeIndex)
        {
            return "NodeIndex";
        }

        if (preset == TestPlacementPreset::ExplicitFrame)
        {
            return "ExplicitFrame";
        }

        return "Unknown";
    }
    void AppController::togglePlannedPreviewDebug()
    {
        plannedPreviewDebugVisible =
            !plannedPreviewDebugVisible;

        auto& preview =
            sim.getManufacturingPlanPreview();

        preview.setShowInsertionMarker(
            plannedPreviewDebugVisible
        );

        preview.setShowInsertionFrame(
            plannedPreviewDebugVisible
        );

        preview.setShowTransformedInsertOverlay(
            false
        );

        std::cout << "[APP PREVIEW DEBUG] "
            << (plannedPreviewDebugVisible ? "ON" : "OFF")
            << std::endl;
    }

   
    // Support for HUD============================
    const char* AppController::activeAttachModeName() const
    {
        if (activePlacementPreset
            != TestPlacementPreset::ExplicitFrame)
        {
            return "-";
        }

        return explicitFrameAttachModeToString(
            activeExplicitAttachMode
        );
    }

    void AppController::configureManufacturingDebug()
    {
        auto& mfgPipe =
            sim.getManufacturingPipe();

        mfgPipe.setDebugAll(
            DEBUG_MANUFACTURING_SIMULATOR
        );

        mfgPipe.setDebugActiveWindow(
            DEBUG_MFG_ACTIVE_WINDOW
        );

        mfgPipe.setDebugBendStep(
            DEBUG_MFG_BEND_STEP
        );

        mfgPipe.setDebugSnapshot(
            DEBUG_MFG_SNAPSHOT
        );
    }

   

    

  

