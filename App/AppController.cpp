#include <sstream>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
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
#include "Core/Geometry/ConstantCurvatureTorsionProfileBuilder.h"
#include "Core/Geometry/SpatialCurveIntegrator.h"
#include "Core/Geometry/SpatialCurveIntegrationResult.h"
#include "Core/Forming/StretchBendingEvaluationResult.h"
#include "Core/Forming/StretchBendingEvaluator.h"
#include "Core/Forming/StretchBendingEvaluationStatus.h"
#include "Core/Forming/StretchBendingProfileBuilder.h"
#include "Core/Forming/StretchBendingFinalProfileBuilder.h"
#include "Core/Forming/StretchBendingActiveZone.h"
#include "Core/Forming/StretchBendingManufacturingState.h"
#include "Core/Forming/StretchBendingManufacturingStateBuilder.h"
#include "Core/Forming/StretchBendingManufacturingStage.h"
#include "Core/Forming/StretchBendingManufacturingStateAdvancer.h"
#include "Core/Forming/StretchBendingManufacturingTiming.h"
#include "Core/Forming/StretchBendingCurrentProfileParameterResolver.h"
#include "Core/Forming/StretchBendingCurrentProfileBuilder.h"
#include "Core/Forming/StretchBendingOperation.h"
#include "Core/Forming/StretchHelixCurrentProfileBuilder.h"
#include "Core/Forming/StretchHelixWrappingStateAdvancer.h"






//#include "Core/Sampling/PipeCurveSampler.h"

// =====================================
// CONSTRUCTOR
// =====================================

namespace
{
    constexpr double TEST_FEED_1_LENGTH = 198.0;
    constexpr double TEST_BEND_RADIUS = 20.0;
    constexpr double TEST_BEND_ANGLE = PI / 2.0;
    constexpr double TEST_FEED_2_LENGTH =
        260.0;

    constexpr double TEST_INCOMING_STOCK_LENGTH = 600.0;

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
    constexpr bool DEBUG_OPERATION_STOP =
        false;
    constexpr bool DEBUG_EXECUTE_ADDITIONAL_PASS_PLACEHOLDER =
        true;

    constexpr bool DEBUG_DEFORMABLE_REGION_SELECTION =
        true;
    constexpr bool DEBUG_TEST_SPATIAL_CURVE_INTEGRATOR =
        true;
    constexpr bool DEBUG_STRETCH_PLAYBACK_EVERY_STEP =
        true;
    // helix spatial test
    constexpr bool DEBUG_TEST_SPATIAL_HELIX_INTEGRATOR =
        true;

    constexpr double TEST_INTEGRATOR_ARC_LENGTH =
        100.0;

    constexpr double TEST_INTEGRATOR_CURVATURE =
        0.02;

    constexpr double TEST_INTEGRATOR_TORSION =
        0.0;

    constexpr double TEST_INTEGRATOR_SAMPLE_STEP =
        0.25;
    //Threshold initial
    constexpr double TEST_MAX_ENDPOINT_POSITION_ERROR =
        0.05; // mm

    constexpr double TEST_MAX_ENDPOINT_TANGENT_ERROR =
        1e-3;

    constexpr double TEST_MAX_INTEGRATED_LENGTH_ERROR =
        1e-6; // mm

    constexpr double TEST_MAX_RELATIVE_POSITION_ERROR =
        1e-3;
   

    // Target circular helix dimensions.
    constexpr double TEST_SPATIAL_HELIX_RADIUS =
        20.0; // mm

    constexpr double TEST_SPATIAL_HELIX_PITCH =
        30.0; // mm per revolution

    constexpr double TEST_SPATIAL_HELIX_TURNS =
        3.0;

    constexpr double TEST_SPATIAL_HELIX_SAMPLE_STEP =
        0.125; // mm

    // Test strech
    constexpr bool DEBUG_TEST_STRETCH_BENDING_INPUT =
        true;

    constexpr double TEST_STRETCH_OUTER_DIAMETER =
        20.0;

    constexpr double TEST_STRETCH_WALL_THICKNESS =
        1.5;

    constexpr double TEST_STRETCH_YOUNG_MODULUS =
        70000.0;

    constexpr double TEST_STRETCH_YIELD_STRESS =
        250.0;

    constexpr double TEST_STRETCH_HARDENING_MODULUS =
        1000.0;

    constexpr double TEST_STRETCH_ALLOWABLE_STRAIN =
        0.08;

    constexpr double TEST_STRETCH_TARGET_LENGTH =
        200.0;

    constexpr double TEST_STRETCH_TARGET_CURVATURE =
        0.002;

    constexpr double TEST_STRETCH_TARGET_TORSION =
        0.0;

    constexpr double TEST_STRETCH_AXIAL_STRAIN =
        0.03;

    constexpr double TEST_STRETCH_FEED_SPEED =
        40.0;

    constexpr double TEST_STRETCH_SAMPLE_STEP =
        0.25;

    constexpr double TEST_STRETCH_SPRINGBACK_RATIO =
        0.10;
   
   
    struct StretchEvaluationTestCase
    {
        const char* name =
            "";

        double targetCurvature =
            0.0;

        double axialStretchStrain =
            0.0;

        StretchBendingEvaluationStatus expectedStatus =
            StretchBendingEvaluationStatus::NotEvaluated;
    };
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
    configureControllerDebug();

    rebuildTestManufacturingPlan();

    configureInitialMode();




    std::cout
        << "[APP PLACEMENT PRESET] "
        << testPlacementPresetToString(
            activePlacementPreset
        )
        << std::endl;

    auto& preview =
        sim.getManufacturingPlanPreview();

    preview.setShowInsertionMarker(
        true
    );

    preview.setShowInsertionFrame(
        false
    );

    preview.setShowTransformedInsertOverlay(
        true
    );
   
    debugTestSpatialCurveIntegrator();
    debugTestSpatialHelixIntegrator();
    debugTestStretchBendingEvaluation();
    debugTestStretchBendingFeasibilityCases();
    debugTestStretchBendingProfileBuilder();
    debugTestStretchBendingGeometry();
    debugTestStretchBendingStateProgression();
    debugTestStretchBendingSpringback();
    debugPrintStretchProcessHudData();
    debugTestStretchBendingOperationValidation();
    debugTestStretchHelixWrappingInput();
    debugTestStretchHelixWrappingKinematics();
    debugTestStretchHelixMechanicsMildCase();
    debugTestStretchHelixContactProgression();
    debugTestStretchHelixWrappingTimeProgression();

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

    data.hasAdditionalPassResult =
        DEBUG_EXECUTE_ADDITIONAL_PASS_PLACEHOLDER;

    if (data.hasAdditionalPassResult)
    {
        data.additionalPassResult =
            sim.getLastAdditionalPassExecutionResult();
    }

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

    const StretchBendingManufacturingState& stretchState =
        debugStretchManufacturingState;

    data.showStretchPlaybackStatus =
        debugStretchPlaybackPrepared;

    data.stretchStage =
        stretchBendingManufacturingStageToString(
            stretchState.stage
        );

    data.stretchElapsedTime =
        stretchState.elapsedTime;

    data.stretchProgress =
        stretchState.processProgress;

    data.stretchTensionFraction =
        stretchState.tensionFraction;

    data.stretchBendingFraction =
        stretchState.bendingFraction;

    data.stretchUnloadingFraction =
        stretchState.unloadingFraction;

    data.stretchGeometryValid =
        debugStretchCurrentIntegrationResult.valid
        && debugStretchCurrentIntegrationResult.isComplete()
        && !debugStretchCurrentIntegrationResult.nodes.empty();

    data.stretchActiveZoneStart =
        debugStretchManufacturingState.activeZone.startS;

    data.stretchActiveZoneEnd =
        debugStretchManufacturingState.activeZone.endS;
    const StretchBendingCurrentProfileParameters
        currentParameters =
        StretchBendingCurrentProfileParameterResolver::
        resolve(
            stretchState,
            debugStretchEvaluationResult
        );

    data.stretchCurrentCurvature =
        currentParameters.isValid()
        ? currentParameters.curvature
        : 0.0;
    data.stretchCommandedTension =
        debugStretchEvaluationResult.commandedTension;

    data.stretchRecommendedTension =
        debugStretchEvaluationResult.recommendedTension;
    data.stretchLoadedCurvature =
        debugStretchEvaluationResult.loadedCurvatureCommand;

    data.stretchFinalCurvature =
        debugStretchEvaluationResult.predictedFinalCurvature;
    data.stretchSpringbackRatio =
        debugStretchEvaluationResult.springbackRatio;

    data.stretchSpringbackValid =
        debugStretchEvaluationResult.springbackPredictionValid;
    data.stretchCurvatureRecovery =
        debugStretchEvaluationResult.loadedCurvatureCommand
        - debugStretchEvaluationResult.predictedFinalCurvature;
    if (debugStretchEvaluationResult.valid)
    {
        data.stretchCommandedTension =
            debugStretchEvaluationResult.commandedTension;

        data.stretchRecommendedTension =
            debugStretchEvaluationResult.recommendedTension;

        data.stretchLoadedCurvature =
            debugStretchEvaluationResult.loadedCurvatureCommand;

        data.stretchFinalCurvature =
            debugStretchEvaluationResult.predictedFinalCurvature;

        data.stretchSpringbackRatio =
            debugStretchEvaluationResult.springbackRatio;

        data.stretchCurvatureRecovery =
            debugStretchEvaluationResult.loadedCurvatureCommand
            - debugStretchEvaluationResult.predictedFinalCurvature;

        data.stretchSpringbackValid =
            debugStretchEvaluationResult.springbackPredictionValid;
    }



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
    {
        sim.reset();

        sim.getManufacturingPipe().setIncomingStockLength(
            TEST_INCOMING_STOCK_LENGTH
        );

        rebuildTestManufacturingPlan();

        break;
    }

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
    case UserAction::ToggleDeformableRegionOverlay:
        toggleDeformableRegionOverlay();
        break;

    case UserAction::ToggleSpatialIntegratorPreview:
        toggleSpatialIntegratorPreview();

        std::cout
            << "[SPATIAL PREVIEW] "
            << (
                spatialIntegratorPreviewVisible
                ? "ON"
                : "OFF"
                )
            << std::endl;

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
    const std::vector<Operation>& ops
) const
{
    // =====================================================
    // PRIMARY ROTARY-DRAW PASS
    // =====================================================

    ManufacturingPass rotaryPass =
        RotaryDrawPassBuilder::buildPass(
            ops,
            "Rotary draw bending pass"
        );

    // =====================================================
    // ADDITIONAL HELIX-FORMING PASS
    // =====================================================

    HelixOperation helixPassOp;

    helixPassOp.inputMode =
        HelixOperation::InputMode::RadiusPitch;

    helixPassOp.length =
        TEST_HELIX_LENGTH;

    helixPassOp.helixRadius =
        TEST_HELIX_RADIUS;

    helixPassOp.pitch =
        TEST_HELIX_PITCH;

    helixPassOp.feedSpeed =
        TEST_HELIX_FEED_SPEED;

    ManufacturingPass helixPass =
        HelixFormingPassBuilder::buildPass(
            helixPassOp,
            "Heating element helix pass"
        );

    // Where the additional pass enters the previously
    // manufactured pipe.
    helixPass.placement =
        buildTestPlacement(
            activePlacementPreset
        );

    // Physical arc-length range that may be deformed by
    // the additional helix-forming pass.
    helixPass.deformableRegion.startArcLength =
        TEST_INSERT_ARC_LENGTH;

    helixPass.deformableRegion.endArcLength =
        TEST_INSERT_ARC_LENGTH
        + TEST_HELIX_LENGTH;

    // =====================================================
    // MANUFACTURING PLAN
    // =====================================================

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

    // ManufacturingHistory is fully populated here.
    // The additional-pass placeholder must run only now.

    if (DEBUG_EXECUTE_ADDITIONAL_PASS_PLACEHOLDER)
    {
        sim.debugExecuteFirstAdditionalPassPlaceholder();
    }

    if (DEBUG_PRINT_MANUFACTURING_HISTORY)
    {
        std::cout
            << "[MFG HISTORY REAL PASSES] primary="
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

void AppController::configureControllerDebug()
{
    sim.setDebugOperationStop(
        DEBUG_OPERATION_STOP
    );

    sim.setDebugDeformableRegionSelection(
        DEBUG_DEFORMABLE_REGION_SELECTION
    );
}

const DeformableRegionSelection&
AppController::getLastDeformableRegionSelection() const
{
    return sim.getLastDeformableRegionSelection();
}

const LocalDeformableRegion&
AppController::getLastLocalDeformableRegion() const
{
    return sim.getLastLocalDeformableRegion();
}




//debug test standalone

void AppController::debugTestSpatialCurveIntegrator()
{
    if (!DEBUG_TEST_SPATIAL_CURVE_INTEGRATOR)
        return;

    // =====================================================
    // TEST START FRAME
    //
    // The curve begins at the world origin and initially
    // travels along +X.
    //
    // Curvature acts toward +Y because N starts along +Y.
    // =====================================================

    Frame startFrame;

    startFrame.P =
        Vec3D{
            0.0,
            -120.0,
            0.0
    };

    startFrame.T =
        Vec3D{
            1.0,
            0.0,
            0.0
    };

    startFrame.N =
        Vec3D{
            0.0,
            1.0,
            0.0
    };

    startFrame.B =
        Vec3D{
            0.0,
            0.0,
            1.0
    };

    // =====================================================
    // BUILD CONSTANT ? / ? PROFILE
    // =====================================================

    CurvatureTorsionProfile profile =
        ConstantCurvatureTorsionProfileBuilder::build(
            TEST_INTEGRATOR_ARC_LENGTH,
            TEST_INTEGRATOR_CURVATURE,
            TEST_INTEGRATOR_TORSION
        );

    // =====================================================
    // INTEGRATE TEMPORARY GEOMETRY
    // =====================================================

    SpatialCurveIntegrator integrator;

    debugPlanarIntegrationResult =
        integrator.integrate(
            startFrame,
            profile,
            TEST_INTEGRATOR_SAMPLE_STEP
        );



    const SpatialCurveIntegrationResult& result =
        debugPlanarIntegrationResult;

    // =====================================================
    // DIAGNOSTICS
    // =====================================================

    std::cout
        << "[SPATIAL INTEGRATOR TEST]"
        << " valid="
        << result.valid
        << " complete="
        << result.isComplete()
        << " nodes="
        << result.nodes.size()
        << " requestedLength="
        << result.requestedArcLength
        << " integratedLength="
        << result.integratedArcLength
        << " requestedSteps="
        << result.requestedStepCount
        << " completedSteps="
        << result.completedStepCount
        << std::endl;

    if (!result.nodes.empty())
    {
        const PipeNode& first =
            result.nodes.front();

        const PipeNode& last =
            result.nodes.back();

        std::cout
            << "[SPATIAL INTEGRATOR ENDPOINT]"
            << " firstP=("
            << first.pos.x << ", "
            << first.pos.y << ", "
            << first.pos.z << ")"
            << " lastP=("
            << last.pos.x << ", "
            << last.pos.y << ", "
            << last.pos.z << ")"
            << std::endl;

        std::cout
            << "[SPATIAL INTEGRATOR END FRAME]"
            << " T=("
            << last.T.x << ", "
            << last.T.y << ", "
            << last.T.z << ")"
            << " N=("
            << last.N.x << ", "
            << last.N.y << ", "
            << last.N.z << ")"
            << " B=("
            << last.B.x << ", "
            << last.B.y << ", "
            << last.B.z << ")"
            << std::endl;
    }
    if (!result.nodes.empty())
    {
        const double radius =
            1.0
            / TEST_INTEGRATOR_CURVATURE;

        const double angle =
            TEST_INTEGRATOR_CURVATURE
            * TEST_INTEGRATOR_ARC_LENGTH;

        Vec3D expectedEnd{
 startFrame.P.x
     + radius * std::sin(angle),

 startFrame.P.y
     + radius * (
         1.0 - std::cos(angle)
     ),

 startFrame.P.z
        };

        Vec3D endpointError =
            result.nodes.back().pos
            - expectedEnd;

        SpatialCurveAccuracyReport accuracy;

        const PipeNode& generatedEnd =
            result.nodes.back();

        Vec3D positionDifference =
            generatedEnd.pos
            - expectedEnd;

        accuracy.endpointPositionError =
            positionDifference.length();

        Vec3D expectedEndTangent{
            std::cos(angle),
            std::sin(angle),
            0.0
        };

        Vec3D tangentDifference =
            generatedEnd.T
            - expectedEndTangent;

        accuracy.endpointTangentError =
            tangentDifference.length();

        accuracy.integratedLengthError =
            std::abs(
                result.integratedArcLength
                - result.requestedArcLength
            );

        if (result.requestedArcLength > 1e-12)
        {
            accuracy.relativePositionError =
                accuracy.endpointPositionError
                / result.requestedArcLength;
        }

        accuracy.positionAccepted =
            accuracy.endpointPositionError
            <= TEST_MAX_ENDPOINT_POSITION_ERROR;

        accuracy.tangentAccepted =
            accuracy.endpointTangentError
            <= TEST_MAX_ENDPOINT_TANGENT_ERROR;

        accuracy.lengthAccepted =
            accuracy.integratedLengthError
            <= TEST_MAX_INTEGRATED_LENGTH_ERROR;

        accuracy.accepted =
            accuracy.positionAccepted
            && accuracy.tangentAccepted
            && accuracy.lengthAccepted
            && accuracy.relativePositionError
            <= TEST_MAX_RELATIVE_POSITION_ERROR;

        const char* accuracyStatus =
            accuracy.accepted
            ? "PASS"
            : "FAIL";

        std::cout
            << "[SPATIAL INTEGRATOR ACCEPTANCE] "
            << accuracyStatus
            << std::endl;

        std::cout
            << "[SPATIAL INTEGRATOR ACCURACY]"
            << " positionError="
            << accuracy.endpointPositionError
            << " tangentError="
            << accuracy.endpointTangentError
            << " lengthError="
            << accuracy.integratedLengthError
            << " relativePositionError="
            << accuracy.relativePositionError
            << " positionAccepted="
            << accuracy.positionAccepted
            << " tangentAccepted="
            << accuracy.tangentAccepted
            << " lengthAccepted="
            << accuracy.lengthAccepted
            << " accepted="
            << accuracy.accepted
            << std::endl;




        std::cout
            << "[SPATIAL INTEGRATOR ERROR]"
            << " expectedEnd=("
            << expectedEnd.x << ", "
            << expectedEnd.y << ", "
            << expectedEnd.z << ")"
            << " errorLength="
            << endpointError.length()
            << std::endl;
    }
}



// spatial helix test


void AppController::debugTestSpatialHelixIntegrator()
{
    if (!DEBUG_TEST_SPATIAL_HELIX_INTEGRATOR)
        return;

    // =====================================================
    // 1. TARGET HELIX PARAMETERS
    // =====================================================

    const double radius =
        TEST_SPATIAL_HELIX_RADIUS;

    const double pitch =
        TEST_SPATIAL_HELIX_PITCH;

    const double turns =
        TEST_SPATIAL_HELIX_TURNS;

    const double b =
        pitch / (2.0 * PI);

    const double q =
        std::sqrt(
            radius * radius
            + b * b
        );

    const double curvature =
        radius
        / (
            radius * radius
            + b * b
            );

    const double torsion =
        b
        / (
            radius * radius
            + b * b
            );

    const double lengthPerTurn =
        2.0 * PI * q;

    const double totalArcLength =
        turns * lengthPerTurn;

    // =====================================================
    // 2. ANALYTICALLY CONSISTENT START FRAME
    //
    // Canonical helix:
    //
    //     x = r cos(u)
    //     y = r sin(u)
    //     z = b u
    //
    // At u = 0:
    //
    //     P = (r, 0, 0)
    // =====================================================

    Frame startFrame;

    startFrame.P =
        Vec3D{
            radius,
            120.0,
            0.0
    };

    startFrame.T =
        Vec3D{
            0.0,
            radius / q,
            b / q
    };

    startFrame.N =
        Vec3D{
            -1.0,
            0.0,
            0.0
    };

    startFrame.B =
        cross(
            startFrame.T,
            startFrame.N
        ).normalized();

    // =====================================================
    // 3. CONSTANT ? / ? PROFILE
    // =====================================================

    CurvatureTorsionProfile profile =
        ConstantCurvatureTorsionProfileBuilder::build(
            totalArcLength,
            curvature,
            torsion
        );

    // =====================================================
    // 4. INTEGRATE TEMPORARY 3D CURVE
    // =====================================================

    SpatialCurveIntegrator integrator;

    debugHelixIntegrationResult =
        integrator.integrate(
            startFrame,
            profile,
            TEST_SPATIAL_HELIX_SAMPLE_STEP
        );


    const SpatialCurveIntegrationResult& result =
        debugHelixIntegrationResult;

    std::cout
        << "[SPATIAL HELIX TEST]"
        << " valid="
        << result.valid
        << " complete="
        << result.isComplete()
        << " radius="
        << radius
        << " pitch="
        << pitch
        << " turns="
        << turns
        << " curvature="
        << curvature
        << " torsion="
        << torsion
        << " requestedLength="
        << totalArcLength
        << " integratedLength="
        << result.integratedArcLength
        << " nodes="
        << result.nodes.size()
        << std::endl;

    if (!result.isComplete()
        || result.nodes.empty())
    {
        std::cout
            << "[SPATIAL HELIX ACCEPTANCE] FAIL"
            << std::endl;

        return;
    }

    // =====================================================
    // 5. ANALYTICAL ENDPOINT
    // =====================================================

    const double finalAngle =
        2.0 * PI * turns;

    Vec3D expectedEnd{
radius * std::cos(finalAngle),
120.0
    + radius * std::sin(finalAngle),
b * finalAngle
    };

    const PipeNode& generatedEnd =
        result.nodes.back();

    const Vec3D positionDifference =
        generatedEnd.pos
        - expectedEnd;

    const double positionError =
        positionDifference.length();

    // After a whole number of turns, X/Y return to the
    // starting angular position and Z rises by turns*pitch.
    std::cout
        << "[SPATIAL HELIX ENDPOINT]"
        << " generated=("
        << generatedEnd.pos.x << ", "
        << generatedEnd.pos.y << ", "
        << generatedEnd.pos.z << ")"
        << " expected=("
        << expectedEnd.x << ", "
        << expectedEnd.y << ", "
        << expectedEnd.z << ")"
        << " positionError="
        << positionError
        << std::endl;

    // =====================================================
    // 6. ANALYTICAL END TANGENT
    // =====================================================

    Vec3D expectedEndTangent{
        -radius
            * std::sin(finalAngle)
            / q,

        radius
            * std::cos(finalAngle)
            / q,

        b / q
    };

    const Vec3D tangentDifference =
        generatedEnd.T
        - expectedEndTangent;

    const double tangentError =
        tangentDifference.length();

    const double lengthError =
        std::abs(
            result.integratedArcLength
            - totalArcLength
        );

    const double relativePositionError =
        positionError
        / totalArcLength;

    // =====================================================
    // 7. INITIAL ACCEPTANCE THRESHOLDS
    //
    // The test is longer and spatial, so use a slightly
    // larger endpoint tolerance than the planar 100 mm test.
    // =====================================================

    constexpr double MAX_HELIX_POSITION_ERROR =
        0.10; // mm

    constexpr double MAX_HELIX_TANGENT_ERROR =
        2e-3;

    constexpr double MAX_HELIX_LENGTH_ERROR =
        1e-6; // mm

    constexpr double MAX_HELIX_RELATIVE_ERROR =
        1e-3; // 0.1%

    const bool positionAccepted =
        positionError
        <= MAX_HELIX_POSITION_ERROR;

    const bool tangentAccepted =
        tangentError
        <= MAX_HELIX_TANGENT_ERROR;

    const bool lengthAccepted =
        lengthError
        <= MAX_HELIX_LENGTH_ERROR;

    const bool relativeAccepted =
        relativePositionError
        <= MAX_HELIX_RELATIVE_ERROR;

    const bool accepted =
        positionAccepted
        && tangentAccepted
        && lengthAccepted
        && relativeAccepted;

    std::cout
        << "[SPATIAL HELIX ACCURACY]"
        << " positionError="
        << positionError
        << " tangentError="
        << tangentError
        << " lengthError="
        << lengthError
        << " relativePositionError="
        << relativePositionError
        << " positionAccepted="
        << positionAccepted
        << " tangentAccepted="
        << tangentAccepted
        << " lengthAccepted="
        << lengthAccepted
        << " relativeAccepted="
        << relativeAccepted
        << " accepted="
        << accepted
        << std::endl;

    std::cout
        << "[SPATIAL HELIX ACCEPTANCE] "
        << (
            accepted
            ? "PASS"
            : "FAIL"
            )
        << std::endl;
}
//Strech bending
StretchBendingProcessInput
AppController::buildTestStretchBendingProcessInput() const
{
    StretchBendingProcessInput input;

    input.pipeSection.outerDiameter =
        TEST_STRETCH_OUTER_DIAMETER;

    input.pipeSection.wallThickness =
        TEST_STRETCH_WALL_THICKNESS;

    input.material.youngModulus =
        TEST_STRETCH_YOUNG_MODULUS;

    input.material.yieldStress =
        TEST_STRETCH_YIELD_STRESS;

    input.material.hardeningModulus =
        TEST_STRETCH_HARDENING_MODULUS;

    input.material.allowableStrain =
        TEST_STRETCH_ALLOWABLE_STRAIN;

    input.geometry.targetArcLength =
        TEST_STRETCH_TARGET_LENGTH;

    input.geometry.targetCurvature =
        TEST_STRETCH_TARGET_CURVATURE;

    input.geometry.targetTorsion =
        TEST_STRETCH_TARGET_TORSION;

    input.axialStretchStrain =
        TEST_STRETCH_AXIAL_STRAIN;

    input.feedSpeed =
        TEST_STRETCH_FEED_SPEED;

    input.sampleStep =
        TEST_STRETCH_SAMPLE_STEP;

    input.springbackRatio =
        TEST_STRETCH_SPRINGBACK_RATIO;


    input.compensateSpringback =
        true;

    input.enabled =
        true;

    return input;
}

void AppController::debugTestStretchBendingEvaluation() const
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    const StretchBendingProcessInput input =
        buildTestStretchBendingProcessInput();

    StretchBendingEvaluator evaluator;

    const StretchBendingEvaluationResult result =
        evaluator.evaluate(
            input
        );

    std::cout
        << "[STRETCH EVALUATION]"
        << " status="
        << stretchBendingEvaluationStatusToString(
            result.status
        )
        << " valid="
        << result.valid
        << " inputValid="
        << result.inputValid
        << " geometryFeasible="
        << result.geometryFeasible
        << " aboveYield="
        << result.aboveYield
        << " innerSafe="
        << result.innerWallSafe
        << " outerSafe="
        << result.outerWallSafe
        << std::endl;

    std::cout
        << "[STRETCH STRAIN]"
        << " yield="
        << result.yieldStrain
        << " bending="
        << result.bendingStrain
        << " axial="
        << result.axialStretchStrain
        << " inner="
        << result.innerWallStrain
        << " outer="
        << result.outerWallStrain
        << " minAxial="
        << result.minimumRequiredAxialStrain
        << " maxAxial="
        << result.maximumAllowedAxialStrain
        << std::endl;

    std::cout
        << "[STRETCH FORCE MOMENT]"
        << " tension="
        << result.axialTension
        << " elasticMoment="
        << result.elasticBendingMoment
        << " innerMargin="
        << result.innerCompressionMargin
        << " outerMargin="
        << result.outerStrainMargin
        << std::endl;

    std::cout
        << "[STRETCH AXIAL COMMAND]"
        << " minStrain="
        << result.minimumRequiredAxialStrain
        << " recommendedStrain="
        << result.recommendedAxialStrain
        << " maxStrain="
        << result.maximumAllowedAxialStrain
        << " range="
        << result.axialStrainRange
        << " commandedStrain="
        << result.axialStretchStrain
        << " commandInsideRange="
        << result.commandedStrainInsideRecommendedRange
        << std::endl;

    std::cout
        << "[STRETCH TENSION COMMAND]"
        << " minTension="
        << result.minimumRequiredTension
        << " recommendedTension="
        << result.recommendedTension
        << " maxTension="
        << result.maximumAllowedTension
        << " commandedTension="
        << result.commandedTension
        << std::endl;

    std::cout
        << "[STRETCH SPRINGBACK]"
        << " targetFinalKappa="
        << result.finalTargetCurvature
        << " ratio="
        << result.springbackRatio
        << " compensationApplied="
        << result.springbackCompensationApplied
        << " loadedKappa="
        << result.loadedCurvatureCommand
        << " predictedFinalKappa="
        << result.predictedFinalCurvature
        << " finalError="
        << result.finalCurvatureError
        << " loadedBendingStrain="
        << result.loadedBendingStrain
        << " predictionValid="
        << result.springbackPredictionValid
        << std::endl;
}

void AppController::debugTestStretchBendingFeasibilityCases() const
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    StretchBendingEvaluator evaluator;

    // =====================================================
    // TEST FOUNDATION
    //
    // Current test material:
    //
    //     D = 20 mm
    //     allowable strain = 0.08
    //     yield strain ? 0.003571
    //
    // bending strain:
    //
    //     epsilon_b = kappa * D / 2
    //
    // For kappa = 0.002:
    //
    //     epsilon_b = 0.02
    //
    // Feasible axial range:
    //
    //     0.02 <= epsilon_0 <= 0.06
    // =====================================================

    const std::vector<StretchEvaluationTestCase> testCases =
    {
        // -------------------------------------------------
        // VALID
        //
        // bending = 0.02
        // axial   = 0.03
        //
        // inner = 0.01
        // outer = 0.05
        //
        // Both walls are safe and outer wall is above yield.
        // -------------------------------------------------
        {
            "Valid",
            0.002,
            0.030,
            StretchBendingEvaluationStatus::Valid
        },

        // -------------------------------------------------
        // INNER-WALL COMPRESSION RISK
        //
        // bending = 0.02
        // axial   = 0.01
        //
        // inner = -0.01
        // outer =  0.03
        //
        // Geometry itself has a feasible axial range, but
        // the selected axial stretch is too low.
        // -------------------------------------------------
        {
            "InnerCompression",
            0.002,
            0.010,
            StretchBendingEvaluationStatus::
                InnerWallCompressionRisk
        },

        // -------------------------------------------------
        // OUTER-WALL STRAIN EXCEEDED
        //
        // bending = 0.02
        // axial   = 0.07
        //
        // inner = 0.05
        // outer = 0.09 > allowable 0.08
        // -------------------------------------------------
        {
            "OuterLimit",
            0.002,
            0.070,
            StretchBendingEvaluationStatus::
                OuterWallStrainExceeded
        },

        // -------------------------------------------------
        // BELOW YIELD
        //
        // bending = 0.001
        // axial   = 0.001
        //
        // inner = 0
        // outer = 0.002
        //
        // outer strain remains below yield strain:
        //
        //     0.002 < 0.003571
        // -------------------------------------------------
        {
            "BelowYield",
            0.0001,
            0.001,
            StretchBendingEvaluationStatus::BelowYield
        }
    };

    size_t passedCount =
        0;

    for (const StretchEvaluationTestCase& testCase :
        testCases)
    {
        StretchBendingProcessInput input =
            buildTestStretchBendingProcessInput();

        input.springbackRatio =
            0.0;

        input.compensateSpringback =
            false;

        input.geometry.targetCurvature =
            testCase.targetCurvature;

        input.axialStretchStrain =
            testCase.axialStretchStrain;

        input.geometry.targetCurvature =
            testCase.targetCurvature;

        input.axialStretchStrain =
            testCase.axialStretchStrain;

        const StretchBendingEvaluationResult result =
            evaluator.evaluate(
                input
            );

        const bool passed =
            result.status
            == testCase.expectedStatus;

        if (passed)
        {
            ++passedCount;
        }

        std::cout
            << "[STRETCH CASE]"
            << " name="
            << testCase.name
            << " expected="
            << stretchBendingEvaluationStatusToString(
                testCase.expectedStatus
            )
            << " actual="
            << stretchBendingEvaluationStatusToString(
                result.status
            )
            << " pass="
            << passed
            << " minAxial="
            << result.minimumRequiredAxialStrain
            << " recommendedAxial="
            << result.recommendedAxialStrain
            << " maxAxial="
            << result.maximumAllowedAxialStrain
            << " commandedInsideRange="
            << result.commandedStrainInsideRecommendedRange
            << " recommendedTension="
            << result.recommendedTension
            << " kappa="
            << result.targetCurvature
            << " bending="
            << result.bendingStrain
            << " axial="
            << result.axialStretchStrain
            << " inner="
            << result.innerWallStrain
            << " outer="
            << result.outerWallStrain
            << " feasible="
            << result.geometryFeasible
            << " aboveYield="
            << result.aboveYield
            << std::endl;

    }

    const bool allPassed =
        passedCount
        == testCases.size();

    std::cout
        << "[STRETCH CASE SUMMARY]"
        << " passed="
        << passedCount
        << "/"
        << testCases.size()
        << " result="
        << (
            allPassed
            ? "PASS"
            : "FAIL"
            )
        << std::endl;
}

void AppController::debugTestStretchBendingProfileBuilder()
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;
    
    // =====================================================
    // BUILD A KNOWN VALID TEST CASE
    //
    // Do not use the original severe kappa=0.01 case,
    // because it is intentionally GeometryNotFeasible.
    // =====================================================

    StretchBendingProcessInput input =
        buildTestStretchBendingProcessInput();

    input.geometry.targetCurvature =
        0.002;

    input.geometry.targetTorsion =
        0.0;

    input.axialStretchStrain =
        0.03;

    StretchBendingEvaluator evaluator;

    const StretchBendingEvaluationResult evaluation =
        evaluator.evaluate(
            input
        );

    debugStretchBendingProfile =
        StretchBendingProfileBuilder::build(
            input,
            evaluation
        );

    std::cout
        << "[STRETCH PROFILE]"
        << " evaluationStatus="
        << stretchBendingEvaluationStatusToString(
            evaluation.status
        )
        << " valid="
        << debugStretchBendingProfile.valid
        << " samples="
        << debugStretchBendingProfile.samples.size()
        << " totalLength="
        << debugStretchBendingProfile.totalArcLength
        << std::endl;

    if (!debugStretchBendingProfile.samples.empty())
    {
        const CurvatureTorsionSample& first =
            debugStretchBendingProfile.samples.front();

        const CurvatureTorsionSample& last =
            debugStretchBendingProfile.samples.back();

        std::cout
            << "[STRETCH PROFILE VALUES]"
            << " firstS="
            << first.arcLength
            << " firstKappa="
            << first.curvature
            << " firstTau="
            << first.torsion
            << " lastS="
            << last.arcLength
            << " lastKappa="
            << last.curvature
            << " lastTau="
            << last.torsion
            << std::endl;
    }
    StretchBendingOperation rejectedOperation =
        buildTestStretchBendingOperation();

    rejectedOperation.targetFinalCurvature =
        0.01;

    rejectedOperation.axialStretchStrain =
        0.02;
    const StretchBendingProcessInput rejectedInput =
        StretchBendingProcessInputBuilder::build(
            rejectedOperation
        );

    const StretchBendingEvaluationResult rejectedEvaluation =
        evaluator.evaluate(
            rejectedInput
        );

    const CurvatureTorsionProfile rejectedProfile =
        StretchBendingProfileBuilder::build(
            rejectedInput,
            rejectedEvaluation
        );

    std::cout
        << "[STRETCH PROFILE REJECTION]"
        << " operationValid="
        << rejectedOperation.isValid()
        << " inputValid="
        << rejectedInput.isValid()
        << " evaluationStatus="
        << stretchBendingEvaluationStatusToString(
            rejectedEvaluation.status
        )
        << " evaluationValid="
        << rejectedEvaluation.valid
        << " profileValid="
        << rejectedProfile.valid
        << " samples="
        << rejectedProfile.samples.size()
        << std::endl;
}

void AppController::debugTestStretchBendingGeometry()
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    // =====================================================
    // 1. BUILD KNOWN VALID STRETCH-BENDING INPUT
    //
    // This is a standalone process prototype. It does not
    // modify rotary-draw playback or manufacturing history.
    // =====================================================

    const StretchBendingOperation operation =
        buildTestStretchBendingOperation();

    const StretchBendingProcessInput input =
        StretchBendingProcessInputBuilder::build(
            operation
        );
    if (!operation.isValid())
    {
        std::cout
            << "[STRETCH OPERATION]"
            << " valid=0"
            << " reason=InvalidOperation"
            << std::endl;

        return;
    }

    if (!input.isValid())
    {
        std::cout
            << "[STRETCH OPERATION]"
            << " operationValid=1"
            << " inputValid=0"
            << " reason=ConversionFailed"
            << std::endl;

        return;
    }

    std::cout
    << "[STRETCH OPERATION]"
    << " operationValid="
    << operation.isValid()
    << " inputValid="
    << input.isValid()
    << " targetKappa="
    << operation.targetFinalCurvature
    << " torsion="
    << operation.targetTorsion
    << " arcLength="
    << operation.arcLength
    << " axialStrain="
    << operation.axialStretchStrain
    << " springbackRatio="
    << operation.springbackRatio
    << std::endl;

StretchBendingEvaluator evaluator;

const StretchBendingEvaluationResult evaluation =
    evaluator.evaluate(
        input
    );
    // =====================================================
    // 2. EVALUATE MATERIAL / PROCESS FEASIBILITY
    //
    // Exactly one evaluator and one evaluation result belong
    // to this function. Previous compiler errors were caused
    // by declaring these variables a second time below.
    // =====================================================
   
  

    if (!evaluation.valid)
    {
        debugStretchLoadedIntegrationResult.clear();
        debugStretchFinalIntegrationResult.clear();

        debugStretchManufacturingState =
            StretchBendingManufacturingState{};

        std::cout
            << "[STRETCH GEOMETRY]"
            << " evaluationStatus="
            << stretchBendingEvaluationStatusToString(
                evaluation.status
            )
            << " result=REJECTED"
            << std::endl;

        return;
    }

    

    debugStretchOperation =
        operation;
    debugStretchEvaluationResult =
        evaluation;

    // =====================================================
    // 3. BUILD LOADED AND FINAL REFERENCE PROFILES
    //
    // loadedProfile:
    //     machine-loaded compensated curvature
    //
    // finalProfile:
    //     predicted curvature after unloading/springback
    // =====================================================

    const CurvatureTorsionProfile loadedProfile =
        StretchBendingProfileBuilder::build(
            input,
            evaluation
        );

    const CurvatureTorsionProfile finalProfile =
        StretchBendingFinalProfileBuilder::build(
            input,
            evaluation
        );

    if (!loadedProfile.valid
        || !finalProfile.valid)
    {
        debugStretchLoadedIntegrationResult.clear();
        debugStretchFinalIntegrationResult.clear();

        debugStretchManufacturingState =
            StretchBendingManufacturingState{};

        std::cout
            << "[STRETCH SHAPE COMPARISON]"
            << " loadedProfileValid="
            << loadedProfile.valid
            << " finalProfileValid="
            << finalProfile.valid
            << " result=REJECTED"
            << std::endl;

        return;
    }

    // =====================================================
    // 4. DEFINE FIXED ACTIVE ZONE
    //
    // Pipe coordinates use centerline arc length:
    //
    //     s=0       s=40             s=160      s=200
    //      |---------|=================|----------|
    //                 fixed active zone
    //
    // The local activeZone is copied into the stored
    // manufacturing state by the builder below.
    // =====================================================

    StretchBendingActiveZone activeZone;

    activeZone.startS =
        40.0;

    activeZone.endS =
        160.0;

    // =====================================================
    // 5. BUILD INITIAL MANUFACTURING STATE
    //
    // Phase 10J creates data only. This state does not yet
    // deform or recolor geometry.
    // =====================================================

    debugStretchManufacturingState =
        StretchBendingManufacturingStateBuilder::buildReadyState(
            input,
            evaluation,
            activeZone
        );


    debugTestStretchBendingCurrentProfileParameters(
        evaluation
    );

    debugTestStretchBendingCurrentProfileBuilder(
        evaluation
    );

    debugTestStretchBendingCurrentGeometry(
        evaluation
    );
    // =====================================================
    // 6. DEFINE A SEPARATE DEBUG START FRAME
    //
    // Keep the standalone stretch result away from:
    //
    //     normal manufacturing pipe
    //     planar integrator test
    //     helix integrator test
    //
    // Start tangent:  +X
    // Bending normal: +Y
    // =====================================================

    Frame startFrame;

    startFrame.P =
        Vec3D{
            0.0,
            -220.0,
            0.0
    };

    startFrame.T =
        Vec3D{
            1.0,
            0.0,
            0.0
    };

    startFrame.N =
        Vec3D{
            0.0,
            1.0,
            0.0
    };

    startFrame.B =
        Vec3D{
            0.0,
            0.0,
            1.0
    };

    // =====================================================
    // 7. INTEGRATE LOADED AND FINAL GEOMETRY
    //
    // Both profiles use the same start frame and sample step.
    // Therefore their visible separation is caused only by
    // the springback curvature difference.
    // =====================================================

    SpatialCurveIntegrator integrator;

    debugStretchLoadedIntegrationResult =
        integrator.integrate(
            startFrame,
            loadedProfile,
            input.sampleStep
        );

    debugStretchFinalIntegrationResult =
        integrator.integrate(
            startFrame,
            finalProfile,
            input.sampleStep
        );

    // Local const-reference aliases improve readability and
    // do not copy either integration result.
    const SpatialCurveIntegrationResult& loadedResult =
        debugStretchLoadedIntegrationResult;

    const SpatialCurveIntegrationResult& finalResult =
        debugStretchFinalIntegrationResult;

    // =====================================================
    // 8. LOADED / FINAL SHAPE DIAGNOSTICS
    // =====================================================

    std::cout
        << "[STRETCH SHAPE COMPARISON]"
        << " loadedValid="
        << loadedResult.valid
        << " loadedComplete="
        << loadedResult.isComplete()
        << " finalValid="
        << finalResult.valid
        << " finalComplete="
        << finalResult.isComplete()
        << " loadedKappa="
        << evaluation.loadedCurvatureCommand
        << " finalKappa="
        << evaluation.predictedFinalCurvature
        << " targetKappa="
        << evaluation.finalTargetCurvature
        << std::endl;

    if (loadedResult.isComplete()
        && finalResult.isComplete()
        && !loadedResult.nodes.empty()
        && !finalResult.nodes.empty())
    {
        const Vec3D loadedEnd =
            loadedResult.nodes.back().pos;

        const Vec3D finalEnd =
            finalResult.nodes.back().pos;

        const Vec3D endpointRecovery =
            finalEnd - loadedEnd;

        std::cout
            << "[STRETCH SPRINGBACK DISPLACEMENT]"
            << " loadedEnd=("
            << loadedEnd.x << ", "
            << loadedEnd.y << ", "
            << loadedEnd.z << ")"
            << " finalEnd=("
            << finalEnd.x << ", "
            << finalEnd.y << ", "
            << finalEnd.z << ")"
            << " endpointRecoveryLength="
            << endpointRecovery.length()
            << std::endl;
    }

    std::cout
        << "[STRETCH GEOMETRY]"
        << " evaluationStatus="
        << stretchBendingEvaluationStatusToString(
            evaluation.status
        )
        << " loadedProfileValid="
        << loadedProfile.valid
        << " resultValid="
        << loadedResult.valid
        << " complete="
        << loadedResult.isComplete()
        << " nodes="
        << loadedResult.nodes.size()
        << " requestedLength="
        << loadedResult.requestedArcLength
        << " integratedLength="
        << loadedResult.integratedArcLength
        << " loadedCurvature="
        << evaluation.loadedCurvatureCommand
        << " torsion="
        << input.geometry.targetTorsion
        << std::endl;

    if (!loadedResult.nodes.empty())
    {
        const PipeNode& first =
            loadedResult.nodes.front();

        const PipeNode& last =
            loadedResult.nodes.back();

        std::cout
            << "[STRETCH GEOMETRY ENDPOINT]"
            << " firstP=("
            << first.pos.x << ", "
            << first.pos.y << ", "
            << first.pos.z << ")"
            << " lastP=("
            << last.pos.x << ", "
            << last.pos.y << ", "
            << last.pos.z << ")"
            << std::endl;
    }

    // =====================================================
    // 9. LOADED GEOMETRY ANALYTICAL ACCURACY
    //
    // Guard nodes.back() with completeness and emptiness.
    // =====================================================

    if (loadedResult.isComplete()
        && !loadedResult.nodes.empty())
    {
        const double curvature =
            evaluation.loadedCurvatureCommand;

        const double length =
            input.geometry.targetArcLength;

        const double angle =
            curvature * length;

        Vec3D expectedEnd;

        if (curvature > 1e-12)
        {
            const double radius =
                1.0 / curvature;

            expectedEnd =
                startFrame.P
                + Vec3D{
                    radius * std::sin(angle),

                    radius
                        * (
                            1.0
                            - std::cos(angle)
                        ),

                    0.0
            };
        }
        else
        {
            expectedEnd =
                startFrame.P
                + startFrame.T * length;
        }

        const Vec3D error =
            loadedResult.nodes.back().pos
            - expectedEnd;

        std::cout
            << "[STRETCH LOADED GEOMETRY ACCURACY]"
            << " loadedKappa="
            << curvature
            << " expectedEnd=("
            << expectedEnd.x << ", "
            << expectedEnd.y << ", "
            << expectedEnd.z << ")"
            << " positionError="
            << error.length()
            << std::endl;
    }

    // =====================================================
    // 10. FINAL UNLOADED GEOMETRY ANALYTICAL ACCURACY
    // =====================================================

    if (finalResult.isComplete()
        && !finalResult.nodes.empty())
    {
        const double finalCurvature =
            evaluation.predictedFinalCurvature;

        const double length =
            input.geometry.targetArcLength;

        const double angle =
            finalCurvature * length;

        Vec3D expectedFinalEnd;

        if (finalCurvature > 1e-12)
        {
            const double radius =
                1.0 / finalCurvature;

            expectedFinalEnd =
                startFrame.P
                + Vec3D{
                    radius * std::sin(angle),

                    radius
                        * (
                            1.0
                            - std::cos(angle)
                        ),

                    0.0
            };
        }
        else
        {
            expectedFinalEnd =
                startFrame.P
                + startFrame.T * length;
        }

        const Vec3D finalError =
            finalResult.nodes.back().pos
            - expectedFinalEnd;

        std::cout
            << "[STRETCH FINAL GEOMETRY ACCURACY]"
            << " finalKappa="
            << finalCurvature
            << " expectedEnd=("
            << expectedFinalEnd.x << ", "
            << expectedFinalEnd.y << ", "
            << expectedFinalEnd.z << ")"
            << " positionError="
            << finalError.length()
            << std::endl;
    }

    // =====================================================
    // 11. PHASE 10J MANUFACTURING-STATE DIAGNOSTICS
    // =====================================================

    const StretchBendingManufacturingState& manufacturingState =
        debugStretchManufacturingState;

    std::cout
        << "[STRETCH MANUFACTURING STATE]"
        << " stage="
        << stretchBendingManufacturingStageToString(
            manufacturingState.stage
        )
        << " progress="
        << manufacturingState.processProgress
        << " tensionFraction="
        << manufacturingState.tensionFraction
        << " bendingFraction="
        << manufacturingState.bendingFraction
        << " unloadingFraction="
        << manufacturingState.unloadingFraction
        << " valid="
        << manufacturingState.isValidForLength(
            input.geometry.targetArcLength
        )
        << std::endl;

    std::cout
        << "[STRETCH ACTIVE ZONE]"
        << " startS="
        << manufacturingState.activeZone.startS
        << " endS="
        << manufacturingState.activeZone.endS
        << " length="
        << manufacturingState.activeZone.length()
        << " totalLength="
        << input.geometry.targetArcLength
        << " valid="
        << manufacturingState.activeZone.isValidForLength(
            input.geometry.targetArcLength
        )
        << std::endl;

    const double beforeZoneS =
        20.0;

    const double insideZoneS =
        100.0;

    const double afterZoneS =
        180.0;

    std::cout
        << "[STRETCH ACTIVE ZONE CLASSIFICATION]"
        << " beforeS="
        << beforeZoneS
        << " beforeActive="
        << manufacturingState.activeZone.contains(
            beforeZoneS
        )
        << " insideS="
        << insideZoneS
        << " insideActive="
        << manufacturingState.activeZone.contains(
            insideZoneS
        )
        << " afterS="
        << afterZoneS
        << " afterActive="
        << manufacturingState.activeZone.contains(
            afterZoneS
        )
        << std::endl;

    // Verify that a zone extending beyond the pipe length is
    // rejected and produces the default Invalid state.
    StretchBendingActiveZone invalidZone;

    invalidZone.startS =
        170.0;

    invalidZone.endS =
        230.0;

    const StretchBendingManufacturingState invalidState =
        StretchBendingManufacturingStateBuilder::buildReadyState(
            input,
            evaluation,
            invalidZone
        );

    std::cout
        << "[STRETCH ACTIVE ZONE REJECTION]"
        << " zoneStart="
        << invalidZone.startS
        << " zoneEnd="
        << invalidZone.endS
        << " pipeLength="
        << input.geometry.targetArcLength
        << " stateStage="
        << stretchBendingManufacturingStageToString(
            invalidState.stage
        )
        << " stateValid="
        << invalidState.isValidForLength(
            input.geometry.targetArcLength
        )
        << std::endl;
}
void AppController::debugTestStretchBendingSpringback() const
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    StretchBendingProcessInput input =
        buildTestStretchBendingProcessInput();

    input.geometry.targetCurvature =
        0.002;

    input.axialStretchStrain =
        0.04;

    input.springbackRatio =
        0.10;

    input.compensateSpringback =
        true;

    StretchBendingEvaluator evaluator;

    const StretchBendingEvaluationResult result =
        evaluator.evaluate(
            input
        );

    const double tolerance =
        1e-12;

    const bool finalCurvatureAccepted =
        std::abs(
            result.predictedFinalCurvature
            - input.geometry.targetCurvature
        )
        <= tolerance;

    std::cout
        << "[STRETCH SPRINGBACK TEST]"
        << " status="
        << stretchBendingEvaluationStatusToString(
            result.status
        )
        << " targetKappa="
        << input.geometry.targetCurvature
        << " loadedKappa="
        << result.loadedCurvatureCommand
        << " predictedFinalKappa="
        << result.predictedFinalCurvature
        << " ratio="
        << result.springbackRatio
        << " error="
        << result.finalCurvatureError
        << " accepted="
        << finalCurvatureAccepted
        << std::endl;
}
void AppController::debugTestStretchBendingStateProgression()
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    // =================================================
    // PHASE 10K — STATE PROGRESSION TEST
    //
    // Work on a copy so the stored Ready state used by
    // rendering remains unchanged.
    // =================================================

    StretchBendingManufacturingState testState =
        debugStretchManufacturingState;

    if (!testState.valid)
    {
        std::cout
            << "[STRETCH STATE PROGRESSION]"
            << " result=REJECTED"
            << " reason=InitialStateInvalid"
            << std::endl;

        return;
    }

    StretchBendingManufacturingTiming timing;

    timing.tensionDuration =
        1.0;

    timing.formingDuration =
        2.0;

    timing.loadedHoldDuration =
        0.5;

    timing.unloadingDuration =
        1.0;

    // A quarter-second step gives several diagnostic
    // samples inside each manufacturing stage.
    constexpr double TEST_DT =
        0.25;

    StretchBendingManufacturingStage previousStage =
        testState.stage;

    std::cout
        << "[STRETCH STATE TRANSITION]"
        << " stage="
        << stretchBendingManufacturingStageToString(
            testState.stage
        )
        << " time="
        << testState.elapsedTime
        << " progress="
        << testState.processProgress
        << std::endl;

    while (testState.stage
        != StretchBendingManufacturingStage::Complete
        && testState.valid)
    {
        StretchBendingManufacturingStateAdvancer::advance(
            testState,
            TEST_DT,
            timing
        );

        // Print only stage transitions. This keeps the
        // console readable while still proving that the
        // complete state machine was traversed.
        if (testState.stage != previousStage)
        {
            std::cout
                << "[STRETCH STATE TRANSITION]"
                << " stage="
                << stretchBendingManufacturingStageToString(
                    testState.stage
                )
                << " time="
                << testState.elapsedTime
                << " progress="
                << testState.processProgress
                << " tensionFraction="
                << testState.tensionFraction
                << " bendingFraction="
                << testState.bendingFraction
                << " unloadingFraction="
                << testState.unloadingFraction
                << std::endl;

            previousStage =
                testState.stage;
        }
    }

    const bool accepted =
        testState.valid
        && testState.stage
        == StretchBendingManufacturingStage::Complete
        && std::abs(
            testState.processProgress - 1.0
        ) <= 1e-12
        && std::abs(
            testState.tensionFraction
        ) <= 1e-12
        && std::abs(
            testState.bendingFraction - 1.0
        ) <= 1e-12
        && std::abs(
            testState.unloadingFraction - 1.0
        ) <= 1e-12;

    std::cout
        << "[STRETCH STATE PROGRESSION SUMMARY]"
        << " stage="
        << stretchBendingManufacturingStageToString(
            testState.stage
        )
        << " elapsedTime="
        << testState.elapsedTime
        << " totalDuration="
        << timing.totalDuration()
        << " progress="
        << testState.processProgress
        << " tensionFraction="
        << testState.tensionFraction
        << " bendingFraction="
        << testState.bendingFraction
        << " unloadingFraction="
        << testState.unloadingFraction
        << " accepted="
        << accepted
        << std::endl;

    }


    void AppController::
debugTestStretchBendingCurrentProfileParameters(
    const StretchBendingEvaluationResult& evaluation)
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    StretchBendingManufacturingState state =
        debugStretchManufacturingState;

    if (!state.isValid())
    {
        std::cout
            << "[STRETCH CURRENT PARAMETERS]"
            << " accepted=0"
            << " reason=InvalidInitialState"
            << std::endl;

        return;
    }

    if (!evaluation.valid)
    {
        std::cout
            << "[STRETCH CURRENT PARAMETERS]"
            << " accepted=0"
            << " reason=InvalidEvaluation"
            << std::endl;

        return;
    }

    auto printParameters =
        [&](const char* testName)
        {
            const StretchBendingCurrentProfileParameters
                parameters =
                    StretchBendingCurrentProfileParameterResolver::
                        resolve(
                            state,
                            evaluation
                        );

            std::cout
                << "[STRETCH CURRENT PARAMETERS]"
                << " test="
                << testName
                << " stage="
                << stretchBendingManufacturingStageToString(
                    state.stage
                )
                << " bendingFraction="
                << state.bendingFraction
                << " unloadingFraction="
                << state.unloadingFraction
                << " curvature="
                << parameters.curvature
                << " torsion="
                << parameters.torsion
                << " valid="
                << parameters.isValid()
                << std::endl;
        };

    state.stage =
        StretchBendingManufacturingStage::Ready;

    state.bendingFraction =
        0.0;

    state.unloadingFraction =
        0.0;

    printParameters(
        "Ready"
    );

    state.stage =
        StretchBendingManufacturingStage::Forming;

    state.bendingFraction =
        0.5;

    state.unloadingFraction =
        0.0;

    printParameters(
        "HalfForming"
    );

    state.stage =
        StretchBendingManufacturingStage::LoadedHold;

    state.bendingFraction =
        1.0;

    state.unloadingFraction =
        0.0;

    printParameters(
        "LoadedHold"
    );

    state.stage =
        StretchBendingManufacturingStage::Unloading;

    state.bendingFraction =
        1.0;

    state.unloadingFraction =
        0.5;

    printParameters(
        "HalfUnloading"
    );

    state.stage =
        StretchBendingManufacturingStage::Complete;

    state.bendingFraction =
        1.0;

    state.unloadingFraction =
        1.0;

    printParameters(
        "Complete"
    );
}
void AppController::
debugTestStretchBendingCurrentProfileBuilder(
    const StretchBendingEvaluationResult& evaluation)
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    StretchBendingManufacturingState state =
        debugStretchManufacturingState;

    if (!state.isValid())
    {
        std::cout
            << "[STRETCH CURRENT PROFILE]"
            << " accepted=0"
            << " reason=InvalidInitialState"
            << std::endl;

        return;
    }

    if (!evaluation.valid)
    {
        std::cout
            << "[STRETCH CURRENT PROFILE]"
            << " accepted=0"
            << " reason=InvalidEvaluation"
            << std::endl;

        return;
    }

    state.stage =
        StretchBendingManufacturingStage::LoadedHold;

    state.bendingFraction =
        1.0;

    state.unloadingFraction =
        0.0;

    const CurvatureTorsionProfile currentProfile =
        StretchBendingCurrentProfileBuilder::build(
            state,
            evaluation
        );

    // =====================================================
// PHASE 10L — CURRENT PROFILE SAMPLE ACCEPTANCE
//
// Expected profile:
//
//     s=0       s=40             s=160       s=200
//      |----------|================|------------|
//          k=0        k=current        k=0
//
// Six samples are expected because each active-zone
// boundary contains two values:
//
//     outside value
//     inside value
//
// This diagnostic verifies profile construction only.
// It does not yet verify how SpatialCurveIntegrator
// resolves duplicate arc-length samples.
// =====================================================

    const double tolerance =
        1e-12;

    bool sampleCountAccepted =
        currentProfile.samples.size()
        == 6;

    bool sampleValuesAccepted =
        false;

    bool sampleOrderAccepted =
        false;

    if (sampleCountAccepted)
    {
        const CurvatureTorsionSample& sample0 =
            currentProfile.samples[0];

        const CurvatureTorsionSample& sample1 =
            currentProfile.samples[1];

        const CurvatureTorsionSample& sample2 =
            currentProfile.samples[2];

        const CurvatureTorsionSample& sample3 =
            currentProfile.samples[3];

        const CurvatureTorsionSample& sample4 =
            currentProfile.samples[4];

        const CurvatureTorsionSample& sample5 =
            currentProfile.samples[5];

        // =================================================
        // PRINT EVERY GENERATED SAMPLE
        // =================================================

        for (std::size_t index = 0;
            index < currentProfile.samples.size();
            ++index)
        {
            const CurvatureTorsionSample& sample =
                currentProfile.samples[index];

            std::cout
                << "[STRETCH CURRENT PROFILE SAMPLE]"
                << " index="
                << index
                << " s="
                << sample.arcLength
                << " curvature="
                << sample.curvature
                << " torsion="
                << sample.torsion
                << std::endl;
        }

        // =================================================
        // VERIFY ARC-LENGTH ORDER
        //
        // Non-decreasing order is required rather than
        // strictly increasing order because duplicate
        // positions are intentional at s=40 and s=160.
        // =================================================

        sampleOrderAccepted =
            sample0.arcLength <= sample1.arcLength
            && sample1.arcLength <= sample2.arcLength
            && sample2.arcLength <= sample3.arcLength
            && sample3.arcLength <= sample4.arcLength
            && sample4.arcLength <= sample5.arcLength;

        // =================================================
        // VERIFY EXACT EXPECTED SAMPLE VALUES
        // =================================================

        const bool sample0Accepted =
            std::abs(
                sample0.arcLength - 0.0
            ) <= tolerance
            && std::abs(
                sample0.curvature - 0.0
            ) <= tolerance
            && std::abs(
                sample0.torsion - 0.0
            ) <= tolerance;

        const bool sample1Accepted =
            std::abs(
                sample1.arcLength
                - state.activeZone.startS
            ) <= tolerance
            && std::abs(
                sample1.curvature - 0.0
            ) <= tolerance
            && std::abs(
                sample1.torsion - 0.0
            ) <= tolerance;

        const bool sample2Accepted =
            std::abs(
                sample2.arcLength
                - state.activeZone.startS
            ) <= tolerance
            && std::abs(
                sample2.curvature
                - evaluation.loadedCurvatureCommand
            ) <= tolerance
            && std::abs(
                sample2.torsion
                - evaluation.targetTorsion
            ) <= tolerance;

        const bool sample3Accepted =
            std::abs(
                sample3.arcLength
                - state.activeZone.endS
            ) <= tolerance
            && std::abs(
                sample3.curvature
                - evaluation.loadedCurvatureCommand
            ) <= tolerance
            && std::abs(
                sample3.torsion
                - evaluation.targetTorsion
            ) <= tolerance;

        const bool sample4Accepted =
            std::abs(
                sample4.arcLength
                - state.activeZone.endS
            ) <= tolerance
            && std::abs(
                sample4.curvature - 0.0
            ) <= tolerance
            && std::abs(
                sample4.torsion - 0.0
            ) <= tolerance;

        const bool sample5Accepted =
            std::abs(
                sample5.arcLength
                - evaluation.targetArcLength
            ) <= tolerance
            && std::abs(
                sample5.curvature - 0.0
            ) <= tolerance
            && std::abs(
                sample5.torsion - 0.0
            ) <= tolerance;

        sampleValuesAccepted =
            sample0Accepted
            && sample1Accepted
            && sample2Accepted
            && sample3Accepted
            && sample4Accepted
            && sample5Accepted;
    }

    const bool profileAcceptance =
        currentProfile.valid
        && sampleCountAccepted
        && sampleOrderAccepted
        && sampleValuesAccepted;

    std::cout
        << "[STRETCH CURRENT PROFILE ACCEPTANCE]"
        << " profileValid="
        << currentProfile.valid
        << " sampleCount="
        << currentProfile.samples.size()
        << " sampleCountAccepted="
        << sampleCountAccepted
        << " sampleOrderAccepted="
        << sampleOrderAccepted
        << " sampleValuesAccepted="
        << sampleValuesAccepted
        << " accepted="
        << profileAcceptance
        << std::endl;

    if (!profileAcceptance)
    {
        std::cout
            << "[STRETCH CURRENT PROFILE SAMPLING SUMMARY]"
            << " accepted=0"
            << " reason=ProfileConstructionFailed"
            << std::endl;

        return;
    }
   

    std::cout
        << "[STRETCH CURRENT PROFILE TEST SUMMARY]"
        << " samples="
        << currentProfile.samples.size()
        << " totalArcLength="
        << currentProfile.totalArcLength
        << " valid="
        << currentProfile.valid
        << " accepted="
        << profileAcceptance
        << std::endl;

    SpatialCurveIntegrator integrator;

    struct ProfileSamplingCase
    {
        const char* name;
        double arcLength;
        double expectedCurvature;
        double expectedTorsion;
    };

    const double boundaryOffset =
        1e-6;

    const std::vector<ProfileSamplingCase> samplingCases =
    {
        {
            "BeforeStart",
            20.0,
            0.0,
            0.0
        },
        {
            "StartLeft",
            state.activeZone.startS
                - boundaryOffset,
            0.0,
            0.0
        },
        {
            "StartExact",
            state.activeZone.startS,
            evaluation.loadedCurvatureCommand,
            evaluation.targetTorsion
        },
        {
            "StartRight",
            state.activeZone.startS
                + boundaryOffset,
            evaluation.loadedCurvatureCommand,
            evaluation.targetTorsion
        },
        {
            "Inside",
            100.0,
            evaluation.loadedCurvatureCommand,
            evaluation.targetTorsion
        },
        {
            "EndLeft",
            state.activeZone.endS
                - boundaryOffset,
            evaluation.loadedCurvatureCommand,
            evaluation.targetTorsion
        },
        {
            "EndExact",
            state.activeZone.endS,
            0.0,
            0.0
        },
        {
            "EndRight",
            state.activeZone.endS
                + boundaryOffset,
            0.0,
            0.0
        },
        {
            "AfterEnd",
            180.0,
            0.0,
            0.0
        }
    };


    std::size_t passedSamplingCases =
        0;

    for (const ProfileSamplingCase& testCase
        : samplingCases)
    {
        double actualCurvature =
            0.0;

        double actualTorsion =
            0.0;

        const bool sampled =
            integrator.sampleProfileForDebug(
                currentProfile,
                testCase.arcLength,
                actualCurvature,
                actualTorsion
            );

        const bool finite =
            std::isfinite(actualCurvature)
            && std::isfinite(actualTorsion);

        const bool curvatureAccepted =
            std::abs(
                actualCurvature
                - testCase.expectedCurvature
            ) <= 1e-10;

        const bool torsionAccepted =
            std::abs(
                actualTorsion
                - testCase.expectedTorsion
            ) <= 1e-10;

        const bool accepted =
            sampled
            && finite
            && curvatureAccepted
            && torsionAccepted;

        if (accepted)
        {
            ++passedSamplingCases;
        }

        std::cout
            << "[STRETCH CURRENT PROFILE SAMPLING]"
            << " case="
            << testCase.name
            << " s="
            << testCase.arcLength
            << " sampled="
            << sampled
            << " curvature="
            << actualCurvature
            << " expectedCurvature="
            << testCase.expectedCurvature
            << " torsion="
            << actualTorsion
            << " expectedTorsion="
            << testCase.expectedTorsion
            << " finite="
            << finite
            << " accepted="
            << accepted
            << std::endl;
    }

    const bool samplingAccepted =
        passedSamplingCases
        == samplingCases.size();

    std::cout
        << "[STRETCH CURRENT PROFILE SAMPLING SUMMARY]"
        << " passed="
        << passedSamplingCases
        << "/"
        << samplingCases.size()
        << " accepted="
        << samplingAccepted
        << std::endl;
}

void AppController::
debugTestStretchBendingCurrentGeometry(
    const StretchBendingEvaluationResult& evaluation)
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    // =================================================
    // 1. COPY THE STORED READY STATE
    //
    // Work on a copy so this diagnostic does not change
    // the manufacturing state used by other tests.
    // =================================================

    StretchBendingManufacturingState state =
        debugStretchManufacturingState;

    if (!state.isValid())
    {
        debugStretchCurrentIntegrationResult.clear();

        std::cout
            << "[STRETCH CURRENT GEOMETRY]"
            << " accepted=0"
            << " reason=InvalidManufacturingState"
            << std::endl;

        return;
    }

    if (!evaluation.valid)
    {
        debugStretchCurrentIntegrationResult.clear();

        std::cout
            << "[STRETCH CURRENT GEOMETRY]"
            << " accepted=0"
            << " reason=InvalidEvaluation"
            << std::endl;

        return;
    }

    // =================================================
    // 2. CONFIGURE A KNOWN LOADED-HOLD STATE
    //
    // LoadedHold gives:
    //
    //     current curvature =
    //         loadedCurvatureCommand
    //
    //     current torsion =
    //         targetTorsion
    // =================================================

    state.stage =
        StretchBendingManufacturingStage::LoadedHold;

    state.bendingFraction =
        1.0;

    state.unloadingFraction =
        0.0;

    state.tensionFraction =
        1.0;

    // =================================================
    // 3. BUILD THE STATE-DRIVEN ACTIVE-ZONE PROFILE
    // =================================================

    const CurvatureTorsionProfile currentProfile =
        StretchBendingCurrentProfileBuilder::build(
            state,
            evaluation
        );

    if (!currentProfile.valid)
    {
        debugStretchCurrentIntegrationResult.clear();

        std::cout
            << "[STRETCH CURRENT GEOMETRY]"
            << " accepted=0"
            << " reason=InvalidCurrentProfile"
            << std::endl;

        return;
    }

    // =================================================
    // 4. DEFINE A SEPARATE DEBUG START FRAME
    //
    // Keep this geometry away from the existing:
    //
    //     planar integrator result
    //     helix result
    //     loaded stretch result
    //     unloaded stretch result
    //
    // Initial direction:
    //     T = +X
    //
    // Bending direction:
    //     N = +Y
    // =================================================

    Frame startFrame;

    startFrame.P =
        Vec3D{
            0.0,
            -320.0,
            0.0
    };

    startFrame.T =
        Vec3D{
            1.0,
            0.0,
            0.0
    };

    startFrame.N =
        Vec3D{
            0.0,
            1.0,
            0.0
    };

    startFrame.B =
        Vec3D{
            0.0,
            0.0,
            1.0
    };


    debugStretchCurrentStartFrame =
        startFrame;

   

    // =================================================
    // 5. INTEGRATE AND STORE CURRENT GEOMETRY
    //
    // Use the same shared integrator as the planar,
    // helix, loaded and final geometry tests.
    // =================================================

    SpatialCurveIntegrator integrator;

    constexpr double CURRENT_GEOMETRY_SAMPLE_STEP =
        0.25;

    debugStretchCurrentSampleStep =
        CURRENT_GEOMETRY_SAMPLE_STEP;

    debugStretchCurrentIntegrationResult =
        integrator.integrate(
            startFrame,
            currentProfile,
            CURRENT_GEOMETRY_SAMPLE_STEP
        );

    const SpatialCurveIntegrationResult& result =
        debugStretchCurrentIntegrationResult;


    debugStretchPlaybackPrepared =
        result.valid
        && result.isComplete()
        && !result.nodes.empty();
   
    // =================================================
    // 6. BASIC RESULT DIAGNOSTIC
    // =================================================

    std::cout
        << "[STRETCH CURRENT GEOMETRY]"
        << " stage="
        << stretchBendingManufacturingStageToString(
            state.stage
        )
        << " profileValid="
        << currentProfile.valid
        << " resultValid="
        << result.valid
        << " complete="
        << result.isComplete()
        << " samples="
        << currentProfile.samples.size()
        << " nodes="
        << result.nodes.size()
        << " requestedLength="
        << result.requestedArcLength
        << " integratedLength="
        << result.integratedArcLength
        << " activeStart="
        << state.activeZone.startS
        << " activeEnd="
        << state.activeZone.endS
        << " curvature="
        << evaluation.loadedCurvatureCommand
        << " torsion="
        << evaluation.targetTorsion
        << std::endl;

    if (!result.valid
        || !result.isComplete()
        || result.nodes.empty())
    {
        debugStretchPlaybackPrepared =
            false;

        std::cout
            << "[STRETCH CURRENT GEOMETRY ACCEPTANCE]"
            << " accepted=0"
            << " reason=IntegrationInvalidOrIncomplete"
            << std::endl;

        return;
    }

    // =================================================
    // 7. ANALYTICAL STRAIGHT–ARC–STRAIGHT SOLUTION
    //
    // Segment 1:
    //
    //     straight length = activeStart
    //
    // Segment 2:
    //
    //     circular arc length =
    //         activeEnd - activeStart
    //
    // Segment 3:
    //
    //     straight length =
    //         totalLength - activeEnd
    // =================================================

    const double totalLength =
        evaluation.targetArcLength;

    const double beforeLength =
        state.activeZone.startS;

    const double activeLength =
        state.activeZone.endS
        - state.activeZone.startS;

    const double afterLength =
        totalLength
        - state.activeZone.endS;

    const double curvature =
        evaluation.loadedCurvatureCommand;

    const double bendAngle =
        curvature
        * activeLength;

    // End of the first straight region.
    const Vec3D beforeEnd =
        startFrame.P
        + startFrame.T
        * beforeLength;

    Vec3D expectedActiveEnd;

    Vec3D expectedFinalTangent;

    if (std::abs(curvature) > 1e-12)
    {
        const double radius =
            1.0
            / curvature;

        // Circular arc displacement expressed in the
        // original T/N frame:
        //
        //     ?P =
        //         T R sin(theta)
        //         +
        //         N R(1-cos(theta))
        expectedActiveEnd =
            beforeEnd
            + startFrame.T
            * (
                radius
                * std::sin(
                    bendAngle
                )
                )
            + startFrame.N
            * (
                radius
                * (
                    1.0
                    - std::cos(
                        bendAngle
                    )
                    )
                );

        expectedFinalTangent =
            startFrame.T
            * std::cos(
                bendAngle
            )
            + startFrame.N
            * std::sin(
                bendAngle
            );
    }
    else
    {
        expectedActiveEnd =
            beforeEnd
            + startFrame.T
            * activeLength;

        expectedFinalTangent =
            startFrame.T;
    }

    // Final straight region follows the tangent produced
    // at the end of the active-zone circular arc.
    const Vec3D expectedEnd =
        expectedActiveEnd
        + expectedFinalTangent
        * afterLength;

    const PipeNode& generatedEnd =
        result.nodes.back();

    const Vec3D endpointDifference =
        generatedEnd.pos
        - expectedEnd;

    const double positionError =
        endpointDifference.length();

    const Vec3D tangentDifference =
        generatedEnd.T
        - expectedFinalTangent;

    const double tangentError =
        tangentDifference.length();

    const double lengthError =
        std::abs(
            result.integratedArcLength
            - totalLength
        );

    // =================================================
    // 8. ACCEPTANCE THRESHOLDS
    //
    // The boundaries align exactly with ds=0.25:
    //
    //     40 / 0.25  = 160 steps
    //     160 / 0.25 = 640 steps
    //
    // Therefore only normal integration error should
    // remain.
    // =================================================

    constexpr double MAX_POSITION_ERROR =
        0.02;

    constexpr double MAX_TANGENT_ERROR =
        1e-3;

    constexpr double MAX_LENGTH_ERROR =
        1e-6;

    const bool positionAccepted =
        positionError
        <= MAX_POSITION_ERROR;

    const bool tangentAccepted =
        tangentError
        <= MAX_TANGENT_ERROR;

    const bool lengthAccepted =
        lengthError
        <= MAX_LENGTH_ERROR;

    const bool finiteAccepted =
        std::isfinite(
            positionError
        )
        && std::isfinite(
            tangentError
        )
        && std::isfinite(
            lengthError
        );

    const bool accepted =
        result.valid
        && result.isComplete()
        && finiteAccepted
        && positionAccepted
        && tangentAccepted
        && lengthAccepted;

    // =================================================
    // 9. FINAL DIAGNOSTICS
    // =================================================

    std::cout
        << "[STRETCH CURRENT GEOMETRY ENDPOINT]"
        << " generated=("
        << generatedEnd.pos.x << ", "
        << generatedEnd.pos.y << ", "
        << generatedEnd.pos.z << ")"
        << " expected=("
        << expectedEnd.x << ", "
        << expectedEnd.y << ", "
        << expectedEnd.z << ")"
        << std::endl;

    std::cout
        << "[STRETCH CURRENT GEOMETRY ACCURACY]"
        << " beforeLength="
        << beforeLength
        << " activeLength="
        << activeLength
        << " afterLength="
        << afterLength
        << " bendAngle="
        << bendAngle
        << " positionError="
        << positionError
        << " tangentError="
        << tangentError
        << " lengthError="
        << lengthError
        << " positionAccepted="
        << positionAccepted
        << " tangentAccepted="
        << tangentAccepted
        << " lengthAccepted="
        << lengthAccepted
        << " finiteAccepted="
        << finiteAccepted
        << std::endl;

    std::cout
        << "[STRETCH CURRENT GEOMETRY ACCEPTANCE] "
        << (
            accepted
            ? "PASS"
            : "FAIL"
            )
        << std::endl;
}
 //getter  
const SpatialCurveIntegrationResult&
AppController::
getDebugStretchCurrentIntegrationResult() const
{
    return debugStretchCurrentIntegrationResult;
}




bool AppController::
rebuildDebugStretchCurrentGeometry()
{
    if (!debugStretchPlaybackPrepared)
    {
        debugStretchCurrentIntegrationResult.clear();
        return false;
    }

    if (!debugStretchManufacturingState.isValid())
    {
        debugStretchCurrentIntegrationResult.clear();
        return false;
    }

    if (!debugStretchEvaluationResult.valid)
    {
        debugStretchCurrentIntegrationResult.clear();
        return false;
    }

    const CurvatureTorsionProfile currentProfile =
        StretchBendingCurrentProfileBuilder::build(
            debugStretchManufacturingState,
            debugStretchEvaluationResult
        );

    if (!currentProfile.valid)
    {
        debugStretchCurrentIntegrationResult.clear();
        return false;
    }

    SpatialCurveIntegrator integrator;

    debugStretchCurrentIntegrationResult =
        integrator.integrate(
            debugStretchCurrentStartFrame,
            currentProfile,
            debugStretchCurrentSampleStep
        );

    return
        debugStretchCurrentIntegrationResult.valid
        && debugStretchCurrentIntegrationResult.isComplete();
}

void AppController::
advanceDebugStretchBendingPlayback(
    double deltaTime)
{
    if (
        debugStretchManufacturingState.stage
        == StretchBendingManufacturingStage::Complete
        )
    {
        std::cout
            << "[STRETCH PLAYBACK STEP]"
            << " ignored=1"
            << " reason=AlreadyComplete"
            << std::endl;

        return;
    }
    if (!debugStretchPlaybackPrepared)
        return;

    if (!std::isfinite(deltaTime))
        return;

    if (deltaTime <= 0.0)
        return;
    const StretchBendingManufacturingStage previousStage =
        debugStretchManufacturingState.stage;

    StretchBendingManufacturingStateAdvancer::advance(
        debugStretchManufacturingState,
        deltaTime,
        debugStretchManufacturingTiming
    );

    const bool geometryRebuilt =
        rebuildDebugStretchCurrentGeometry();

    if (
        debugStretchManufacturingState.stage
        != previousStage
        )
    {
        std::cout
            << "[STRETCH PLAYBACK TRANSITION]"
            << " from="
            << stretchBendingManufacturingStageToString(
                previousStage
            )
            << " to="
            << stretchBendingManufacturingStageToString(
                debugStretchManufacturingState.stage
            )
            << " time="
            << debugStretchManufacturingState.elapsedTime
            << " geometryValid="
            << geometryRebuilt
            << std::endl;
    }

    if (DEBUG_STRETCH_PLAYBACK_EVERY_STEP)
    {
        std::cout
            << "[STRETCH PLAYBACK STEP]"
            << " stage="
            << stretchBendingManufacturingStageToString(
                debugStretchManufacturingState.stage
            )
            << " time="
            << debugStretchManufacturingState.elapsedTime
            << " progress="
            << debugStretchManufacturingState.processProgress
            << " tensionFraction="
            << debugStretchManufacturingState.tensionFraction
            << " bendingFraction="
            << debugStretchManufacturingState.bendingFraction
            << " unloadingFraction="
            << debugStretchManufacturingState.unloadingFraction
            << " geometryValid="
            << debugStretchCurrentIntegrationResult.valid
            << " nodes="
            << debugStretchCurrentIntegrationResult.nodes.size()
            << std::endl;

    }

    if (
        debugStretchManufacturingState.stage
        != previousStage
        )
    {
        const StretchBendingCurrentProfileParameters
            currentParameters =
            StretchBendingCurrentProfileParameterResolver::
            resolve(
                debugStretchManufacturingState,
                debugStretchEvaluationResult
            );

        std::cout
            << "[STRETCH PROCESS TRANSITION DATA]"
            << " stage="
            << stretchBendingManufacturingStageToString(
                debugStretchManufacturingState.stage
            )
            << " tension="
            << debugStretchEvaluationResult.commandedTension
            << " currentKappa="
            << currentParameters.curvature
            << " loadedKappa="
            << debugStretchEvaluationResult.loadedCurvatureCommand
            << " finalKappa="
            << debugStretchEvaluationResult.predictedFinalCurvature
            << std::endl;
    }


}


void AppController::
resetDebugStretchBendingPlayback()
{
    if (!debugStretchEvaluationResult.valid)
        return;

    StretchBendingProcessInput input =
        buildTestStretchBendingProcessInput();

    input.geometry.targetCurvature =
        0.002;

    input.geometry.targetTorsion =
        0.0;

    input.geometry.targetArcLength =
        200.0;

    input.axialStretchStrain =
        0.03;

    input.sampleStep =
        debugStretchCurrentSampleStep;

    StretchBendingActiveZone activeZone;

    activeZone.startS =
        40.0;

    activeZone.endS =
        160.0;

    debugStretchManufacturingState =
        StretchBendingManufacturingStateBuilder::
        buildReadyState(
            input,
            debugStretchEvaluationResult,
            activeZone
        );

    rebuildDebugStretchCurrentGeometry();

    std::cout
        << "[STRETCH PLAYBACK RESET]"
        << " stage="
        << stretchBendingManufacturingStageToString(
            debugStretchManufacturingState.stage
        )
        << " time="
        << debugStretchManufacturingState.elapsedTime
        << std::endl;


   
}

bool AppController::
isDebugStretchBendingPlaybackComplete() const
{
    return
        debugStretchManufacturingState.stage
        == StretchBendingManufacturingStage::Complete;
}

const StretchBendingManufacturingState&

AppController::
getDebugStretchManufacturingState() const
{
    return debugStretchManufacturingState;
}

const StretchBendingEvaluationResult&
AppController::
getDebugStretchEvaluationResult() const
{
    return debugStretchEvaluationResult;
}

bool AppController::
isDebugStretchPlaybackPrepared() const
{
    return debugStretchPlaybackPrepared;
}

bool AppController::
isDebugStretchCurrentGeometryValid() const
{
    return
        debugStretchCurrentIntegrationResult.valid
        && debugStretchCurrentIntegrationResult.isComplete()
        && !debugStretchCurrentIntegrationResult.nodes.empty();
}

const StretchBendingActiveZone&
AppController::getDebugStretchActiveZone() const
{
    return debugStretchManufacturingState.activeZone;
}


void AppController::
debugPrintStretchProcessHudData() const
{
    const HUDData data =
        buildHUDData();

    std::cout
        << "[STRETCH PROCESS DATA]"
        << " stage="
        << data.stretchStage
        << " commandedTension="
        << data.stretchCommandedTension
        << " recommendedTension="
        << data.stretchRecommendedTension
        << " currentKappa="
        << data.stretchCurrentCurvature
        << " loadedKappa="
        << data.stretchLoadedCurvature
        << " finalKappa="
        << data.stretchFinalCurvature
        << " springbackRatio="
        << data.stretchSpringbackRatio
        << " recovery="
        << data.stretchCurvatureRecovery
        << " springbackValid="
        << data.stretchSpringbackValid
        << std::endl;
}

StretchBendingOperation
AppController::buildTestStretchBendingOperation() const
{
  

    // Reuse your existing known-valid section and material
    // values here.
    StretchBendingOperation operation;

    operation.pipeSection.outerDiameter =
        TEST_STRETCH_OUTER_DIAMETER;

    operation.pipeSection.wallThickness =
        TEST_STRETCH_WALL_THICKNESS;

    operation.material.youngModulus =
        TEST_STRETCH_YOUNG_MODULUS;

    operation.material.yieldStress =
        TEST_STRETCH_YIELD_STRESS;

    operation.material.hardeningModulus =
        TEST_STRETCH_HARDENING_MODULUS;

    operation.material.allowableStrain =
        TEST_STRETCH_ALLOWABLE_STRAIN;
    //geometry
    operation.arcLength =
        TEST_STRETCH_TARGET_LENGTH;

    operation.targetFinalCurvature =
        TEST_STRETCH_TARGET_CURVATURE;

    operation.targetTorsion =
        TEST_STRETCH_TARGET_TORSION;
    //process parameters
    operation.axialStretchStrain =
        TEST_STRETCH_AXIAL_STRAIN;

    operation.feedSpeed =
        TEST_STRETCH_FEED_SPEED;

    operation.sampleStep =
        TEST_STRETCH_SAMPLE_STEP;

    operation.springbackRatio =
        TEST_STRETCH_SPRINGBACK_RATIO;

    operation.compensateSpringback =
        true;

    operation.enabled =
        true;


   
    
    return operation;
}


void AppController::
debugTestStretchBendingOperationValidation() const
{
    const StretchBendingOperation valid =
        buildTestStretchBendingOperation();

    StretchBendingOperation invalidLength =
        valid;

    invalidLength.arcLength =
        0.0;

    StretchBendingOperation invalidSpringback =
        valid;

    invalidSpringback.springbackRatio =
        1.0;

    StretchBendingOperation invalidSpeed =
        valid;

    invalidSpeed.feedSpeed =
        0.0;

    std::cout
        << "[STRETCH OPERATION CASE]"
        << " name=Valid"
        << " accepted="
        << valid.isValid()
        << std::endl;

    std::cout
        << "[STRETCH OPERATION CASE]"
        << " name=InvalidLength"
        << " accepted="
        << !invalidLength.isValid()
        << std::endl;

    std::cout
        << "[STRETCH OPERATION CASE]"
        << " name=InvalidSpringback"
        << " accepted="
        << !invalidSpringback.isValid()
        << std::endl;

    std::cout
        << "[STRETCH OPERATION CASE]"
        << " name=InvalidSpeed"
        << " accepted="
        << !invalidSpeed.isValid()
        << std::endl;
}

StretchHelixWrappingInput
AppController::buildTestStretchHelixWrappingInput() const
{
    StretchHelixWrappingInput input;

    // =================================================
    // WORKPIECE
    // =================================================

    input.pipeSection.outerDiameter =
        TEST_STRETCH_OUTER_DIAMETER;

    input.pipeSection.wallThickness =
        TEST_STRETCH_WALL_THICKNESS;

    input.material.youngModulus =
        TEST_STRETCH_YOUNG_MODULUS;

    input.material.yieldStress =
        TEST_STRETCH_YIELD_STRESS;

    input.material.hardeningModulus =
        TEST_STRETCH_HARDENING_MODULUS;

    input.material.allowableStrain =
        TEST_STRETCH_ALLOWABLE_STRAIN;

    input.pipeArcLength =
        500.0;

    // =================================================
    // SUPPORT
    // =================================================

    input.supportOuterRadius =
        50.0;
    
    // =================================================
    // MACHINE MOTION
    // =================================================

    input.axialSpeed =
        20.0;

    input.rotationSpeed =
        2.0;

    input.rotationDirection =
        1;

    // =================================================
    // STRETCH
    // =================================================

    input.axialStretchStrain =
        0.03;

    // =================================================
    // NUMERICS
    // =================================================

    input.sampleStep =
        0.25;

    input.enabled =
        true;

    return input;
}

void AppController::
debugTestStretchHelixWrappingInput() const
{
    const StretchHelixWrappingInput valid =
        buildTestStretchHelixWrappingInput();
   

    StretchHelixWrappingInput invalidRadius =
        valid;

    invalidRadius.supportOuterRadius =
        0.0;

    
    StretchHelixWrappingInput invalidRotation =
        valid;

    invalidRotation.rotationSpeed =
        0.0;

    StretchHelixWrappingInput invalidDirection =
        valid;

    invalidDirection.rotationDirection =
        0;

    StretchHelixWrappingInput invalidLength =
        valid;

    invalidLength.pipeArcLength =
        0.0;

    std::cout
        << "[STRETCH HELIX INPUT CASE]"
        << " name=Valid"
        << " accepted="
        << valid.isValid()
        << std::endl;

    std::cout
        << "[STRETCH HELIX INPUT CASE]"
        << " name=InvalidRadius"
        << " accepted="
        << !invalidRadius.isValid()
        << std::endl;

    std::cout
        << "[STRETCH HELIX INPUT CASE]"
        << " name=InvalidRotation"
        << " accepted="
        << !invalidRotation.isValid()
        << std::endl;

    std::cout
        << "[STRETCH HELIX INPUT CASE]"
        << " name=InvalidDirection"
        << " accepted="
        << !invalidDirection.isValid()
        << std::endl;

    std::cout
        << "[STRETCH HELIX INPUT CASE]"
        << " name=InvalidLength"
        << " accepted="
        << !invalidLength.isValid()
        << std::endl;
}

void AppController::
debugTestStretchHelixWrappingKinematics() 
{
    const StretchHelixWrappingInput input =
        buildTestStretchHelixWrappingInput();
    

    const StretchHelixWrappingKinematics kinematics =
        StretchHelixWrappingKinematicsBuilder::build(
            input
        );



    const double expectedCenterlineSpeed =
        std::sqrt(
            std::pow(
                kinematics.centerlineRadius
                * std::abs(
                    input.rotationSpeed
                ),
                2.0
            )
            +
            std::pow(
                input.axialSpeed,
                2.0
            )
        );
    constexpr double speedTolerance =
        1e-9;
    const bool centerlineSpeedAccepted =
        std::abs(
            expectedCenterlineSpeed
            - kinematics.centerlineSpeed
        ) <= speedTolerance;

    std::cout
        << "[STRETCH HELIX KINEMATICS]"
        << " inputValid="
        << input.isValid()
        << " valid="
        << kinematics.valid
        << " supportRadius="
        << input.supportOuterRadius
        << " pipeOD="
        << input.pipeSection.outerDiameter
        << " centerlineRadius="
        << kinematics.centerlineRadius
        << " axialSpeed="
        << input.axialSpeed
        << " rotationSpeed="
        << input.rotationSpeed
        << " direction="
        << input.rotationDirection
        << " pitch="
        << kinematics.pitch
        << " b="
        << kinematics.helixRisePerRadian
        << " curvature="
        << kinematics.curvature
        << " torsion="
        << kinematics.torsion
        << " helixAngle="
        << kinematics.helixAngle
        << " lengthPerRev="
        << kinematics.arcLengthPerRevolution
        << " centerlineSpeed="
        << kinematics.centerlineSpeed
        << " speedAccepted="
        << centerlineSpeedAccepted
        << std::endl;
        


    if (!input.isValid()
        || !kinematics.valid)
    {
        return;
    }

    debugStretchHelixWrappingInput =
        input;

    debugStretchHelixWrappingKinematics =
        kinematics;

    const CurvatureTorsionProfile referenceProfile =
        ConstantCurvatureTorsionProfileBuilder::build(
            input.pipeArcLength,
            kinematics.curvature,
            kinematics.torsion
        );


    // =====================================================
    // H3.5 — VALIDATE REFERENCE PROFILE
    // =====================================================

    if (!referenceProfile.valid)
    {
        std::cout
            << "[STRETCH HELIX REFERENCE PROFILE]"
            << " valid=0"
            << std::endl;

        return;
    }


    std::cout
        << "[STRETCH HELIX REFERENCE PROFILE]"
        << " valid="
        << referenceProfile.valid
        << " samples="
        << referenceProfile.samples.size()
        << " length="
        << referenceProfile.totalArcLength
        << " curvature="
        << kinematics.curvature
        << " torsion="
        << kinematics.torsion
        << std::endl;

//// =====================================================
    // H3.6 comes HERE
    // Build referenceStartFrame
    // =====================================================
    Frame referenceStartFrame;

    

    referenceStartFrame.P =
        Vec3D{
            0.0,
            -500.0,
            0.0
    };

    referenceStartFrame.T =
        Vec3D{
            0.0,
            1.0,
            0.0
    };

    referenceStartFrame.N =
        Vec3D{
            -1.0,
            0.0,
            0.0
    };

    referenceStartFrame.B =
        Vec3D{
            0.0,
            0.0,
            1.0
    };
    // =====================================================
// STORE THE SAME FRAME FOR H4
//
// H3 yellow reference and H4 orange current geometry
// must begin from exactly the same coordinate frame.
// =====================================================

    debugStretchHelixReferenceStartFrame =
        referenceStartFrame;

    // =====================================================
       // H3.7 comes after H3.6
       // Integrate referenceProfile
       // =====================================================

    SpatialCurveIntegrator integrator;

    debugStretchHelixReferenceResult =
        integrator.integrate(
            debugStretchHelixReferenceStartFrame,
            referenceProfile,
            input.sampleStep
        );

    std::cout
        << "[STRETCH HELIX REFERENCE]"
        << " valid="
        << debugStretchHelixReferenceResult.valid
        << " complete="
        << debugStretchHelixReferenceResult.isComplete()
        << " nodes="
        << debugStretchHelixReferenceResult.nodes.size()
        << " length="
        << debugStretchHelixReferenceResult.integratedArcLength
        << std::endl;

    if (!debugStretchHelixReferenceResult.valid
        || !debugStretchHelixReferenceResult.isComplete())
    {
        return;
    }

    // =====================================================
// H6A.11 — INITIALIZE NEW PROCESS
//
// At this point the old H3 reference path is known-good.
// Now initialize the new process using exactly the same
// input and exactly the same start frame.
// =====================================================

    const bool processInitialized =
        debugStretchHelixProcess.initialize(
            input,
            referenceStartFrame
        );

    std::cout
        << "[STRETCH HELIX PROCESS INIT]"
        << " initialized="
        << processInitialized
        << " valid="
        << debugStretchHelixProcess.isValid()
        << " referenceNodes="
        << debugStretchHelixProcess
        .getReferenceResult()
        .nodes.size()
        << " currentNodes="
        << debugStretchHelixProcess
        .getCurrentNodes()
        .size()
        << std::endl;

    if (!processInitialized)
    {
        return;
    }
    const StretchBendingEvaluationResult& mechanics =
        debugStretchHelixProcess.getStretchEvaluation();

    std::cout
        << "[STRETCH HELIX MECHANICS ACCEPTANCE]"
        << " processValid="
        << debugStretchHelixProcess.isValid()
        << " mechanicsFeasible="
        << debugStretchHelixProcess.isMechanicallyFeasible()
        << " evaluationValid="
        << mechanics.valid
        << " status="
        << stretchBendingEvaluationStatusToString(
            mechanics.status
        )
        << std::endl;
    debugTestStretchHelixProcessAcceptance();
    debugStretchHelixWrappingState =
        StretchHelixWrappingStateBuilder::buildInitial(
            debugStretchHelixWrappingInput
        );
    if (!debugStretchHelixWrappingState.isValidForLength(
        debugStretchHelixWrappingInput.pipeArcLength
    ))
    {
        std::cout
            << "[STRETCH HELIX WRAPPING STATE]"
            << " valid=0"
            << std::endl;
       
        return;
    }

   

    const double denominator =
        kinematics.centerlineRadius
        * kinematics.centerlineRadius
        + kinematics.helixRisePerRadian
        * kinematics.helixRisePerRadian;

    const double reconstructedKappa =
        kinematics.centerlineRadius
        / denominator;

    const double reconstructedTauMagnitude =
        kinematics.helixRisePerRadian
        / denominator;

    constexpr double tolerance =
        1e-12;

    const bool curvatureAccepted =
        std::abs(
            reconstructedKappa
            - kinematics.curvature
        ) <= tolerance;

    const bool torsionAccepted =
        std::abs(
            reconstructedTauMagnitude
            - std::abs(
                kinematics.torsion
            )
        ) <= tolerance;

    const bool accepted =
        kinematics.valid
        && curvatureAccepted
        && torsionAccepted;

    std::cout
        << "[STRETCH HELIX KINEMATICS ACCEPTANCE]"
        << " curvatureAccepted="
        << curvatureAccepted
        << " torsionAccepted="
        << torsionAccepted
        << " accepted="
        << accepted
        << std::endl;

    StretchHelixWrappingInput oppositeInput =
        input;

    oppositeInput.rotationDirection =
        -1;

    const StretchHelixWrappingKinematics opposite =
        StretchHelixWrappingKinematicsBuilder::build(
            oppositeInput
        );

    const bool handednessAccepted =
        opposite.valid
        && std::abs(
            opposite.curvature
            - kinematics.curvature
        ) <= tolerance
        && std::abs(
            opposite.torsion
            + kinematics.torsion
        ) <= tolerance;

    std::cout
        << "[STRETCH HELIX HANDEDNESS]"
        << " positiveTau="
        << kinematics.torsion
        << " negativeTau="
        << opposite.torsion
        << " accepted="
        << handednessAccepted
        << std::endl;



}


void AppController::
debugTestStretchHelixMechanicsMildCase()
{
    // =====================================================
    // H8.11 — KNOWN-MILD MECHANICS CASE
    //
    // Purpose:
    // prove that the mechanical evaluator can accept
    // a much gentler helix than the current R=60 mm case.
    // =====================================================

    StretchHelixWrappingInput mildInput =
        buildTestStretchHelixWrappingInput();

    mildInput.supportOuterRadius =
        500.0;

    if (!mildInput.isValid())
    {
        std::cout
            << "[STRETCH HELIX MILD MECHANICS]"
            << " accepted=0"
            << " reason=InvalidInput"
            << std::endl;

        return;
    }

    const StretchHelixWrappingKinematics mildKinematics =
        StretchHelixWrappingKinematicsBuilder::build(
            mildInput
        );

    if (!mildKinematics.valid)
    {
        std::cout
            << "[STRETCH HELIX MILD MECHANICS]"
            << " accepted=0"
            << " reason=InvalidKinematics"
            << std::endl;

        return;
    }

    StretchBendingProcessInput mechanicalInput;

    mechanicalInput.pipeSection =
        mildInput.pipeSection;

    mechanicalInput.material =
        mildInput.material;

    mechanicalInput.geometry.targetArcLength =
        mildInput.pipeArcLength;

    mechanicalInput.geometry.targetCurvature =
        mildKinematics.curvature;

    mechanicalInput.geometry.targetTorsion =
        mildKinematics.torsion;

    mechanicalInput.axialStretchStrain =
        mildInput.axialStretchStrain;

    mechanicalInput.feedSpeed =
        mildInput.axialSpeed;

    mechanicalInput.sampleStep =
        mildInput.sampleStep;

    mechanicalInput.springbackRatio =
        0.0;

    mechanicalInput.compensateSpringback =
        false;

    mechanicalInput.enabled =
        true;

    if (!mechanicalInput.isValid())
    {
        std::cout
            << "[STRETCH HELIX MILD MECHANICS]"
            << " accepted=0"
            << " reason=InvalidMechanicalInput"
            << std::endl;

        return;
    }

    StretchBendingEvaluator evaluator;

    const StretchBendingEvaluationResult evaluation =
        evaluator.evaluate(
            mechanicalInput
        );

    std::cout
        << "[STRETCH HELIX MILD MECHANICS]"
        << " supportRadius="
        << mildInput.supportOuterRadius
        << " centerlineRadius="
        << mildKinematics.centerlineRadius
        << " curvature="
        << mildKinematics.curvature
        << " torsion="
        << mildKinematics.torsion
        << " evaluationValid="
        << evaluation.valid
        << " status="
        << stretchBendingEvaluationStatusToString(
            evaluation.status
        )
        << " accepted="
        << evaluation.valid
        << std::endl;
}
//getters implemention
const StretchHelixWrappingInput&
AppController::
getDebugStretchHelixWrappingInput() const
{
    return debugStretchHelixWrappingInput;
}

const StretchHelixWrappingKinematics&
AppController::
getDebugStretchHelixWrappingKinematics() const
{
    return debugStretchHelixWrappingKinematics;
}





void AppController::
debugTestStretchHelixContactProgression()
{
    const double fractions[] =
    {
        0.0,
        0.25,
        0.50,
        0.75,
        1.0
    };

    const double totalLength =
        debugStretchHelixWrappingInput.pipeArcLength;

    for (double fraction : fractions)
    {
        debugStretchHelixWrappingState.progress =
            fraction;

        debugStretchHelixWrappingState.wrappedLength =
            totalLength
            * fraction;

        debugStretchHelixWrappingState.contactFrontS =
            debugStretchHelixWrappingState.wrappedLength;

        debugStretchHelixWrappingState.complete =
            fraction >= 1.0;

        debugStretchHelixWrappingState.valid =
            true;

        const bool geometryValid =
            rebuildDebugStretchHelixCurrentGeometry();

        std::cout
            << "[STRETCH HELIX CONTACT]"
            << " progress="
            << fraction
            << " wrappedLength="
            << debugStretchHelixWrappingState.wrappedLength
            << " frontS="
            << debugStretchHelixWrappingState.contactFrontS
            << " geometryValid="
            << geometryValid
            << " nodes="
            << debugStretchHelixCurrentResult.nodes.size()
            << std::endl;
    }

    // Return test state to zero.
    debugStretchHelixWrappingState =
        StretchHelixWrappingStateBuilder::buildInitial(
            debugStretchHelixWrappingInput
        );

    rebuildDebugStretchHelixCurrentGeometry();
}


const SpatialCurveIntegrationResult&
AppController::
getDebugStretchHelixCurrentResult() const
{
    return debugStretchHelixCurrentResult;
}

bool AppController::
rebuildDebugStretchHelixCurrentGeometry()
{
    debugStretchHelixCurrentProfile.clear();
    debugStretchHelixCurrentResult.clear();

    const StretchHelixWrappingInput& input =
        debugStretchHelixWrappingInput;

    const StretchHelixWrappingKinematics& kinematics =
        debugStretchHelixWrappingKinematics;

    const StretchHelixWrappingState& state =
        debugStretchHelixWrappingState;

    // =====================================================
    // 1. INPUT
    // =====================================================

    if (!input.isValid())
    {
        std::cout
            << "[STRETCH HELIX CURRENT REBUILD]"
            << " accepted=0"
            << " reason=InvalidInput"
            << std::endl;

        return false;
    }

    // =====================================================
    // 2. KINEMATICS
    // =====================================================

    if (!kinematics.valid)
    {
        std::cout
            << "[STRETCH HELIX CURRENT REBUILD]"
            << " accepted=0"
            << " reason=InvalidKinematics"
            << std::endl;

        return false;
    }

    // =====================================================
    // 3. WRAPPING STATE
    // =====================================================

    if (!state.isValidForLength(
        input.pipeArcLength
    ))
    {
        std::cout
            << "[STRETCH HELIX CURRENT REBUILD]"
            << " accepted=0"
            << " reason=InvalidWrappingState"
            << " wrappedLength="
            << state.wrappedLength
            << " frontS="
            << state.contactFrontS
            << " progress="
            << state.progress
            << " totalLength="
            << input.pipeArcLength
            << " stateValid="
            << state.valid
            << std::endl;

        return false;
    }

    // =====================================================
    // 4. BUILD CURRENT PROFILE
    // =====================================================

    debugStretchHelixCurrentProfile =
        StretchHelixCurrentProfileBuilder::build(
            input,
            kinematics,
            state
        );

    if (!debugStretchHelixCurrentProfile.valid)
    {
        std::cout
            << "[STRETCH HELIX CURRENT REBUILD]"
            << " accepted=0"
            << " reason=InvalidCurrentProfile"
            << " samples="
            << debugStretchHelixCurrentProfile.samples.size()
            << " frontS="
            << state.contactFrontS
            << std::endl;

        return false;
    }

    std::cout
        << "[STRETCH HELIX CURRENT PROFILE]"
        << " valid=1"
        << " samples="
        << debugStretchHelixCurrentProfile.samples.size()
        << " length="
        << debugStretchHelixCurrentProfile.totalArcLength
        << " frontS="
        << state.contactFrontS
        << std::endl;

    // =====================================================
    // 5. INTEGRATE
    // =====================================================
    const Frame& frame =
        debugStretchHelixReferenceStartFrame;

    std::cout
        << "[STRETCH HELIX CURRENT FRAME]"
        << " P=("
        << frame.P.x << ", "
        << frame.P.y << ", "
        << frame.P.z << ")"
        << " T=("
        << frame.T.x << ", "
        << frame.T.y << ", "
        << frame.T.z << ")"
        << " N=("
        << frame.N.x << ", "
        << frame.N.y << ", "
        << frame.N.z << ")"
        << " B=("
        << frame.B.x << ", "
        << frame.B.y << ", "
        << frame.B.z << ")"
        << std::endl;
    SpatialCurveIntegrator integrator;

    debugStretchHelixCurrentResult =
        integrator.integrate(
            debugStretchHelixReferenceStartFrame,
            debugStretchHelixCurrentProfile,
            input.sampleStep
        );

    if (!debugStretchHelixCurrentResult.valid)
    {
        std::cout
            << "[STRETCH HELIX CURRENT REBUILD]"
            << " accepted=0"
            << " reason=IntegrationInvalid"
            << " samples="
            << debugStretchHelixCurrentProfile.samples.size()
            << " nodes="
            << debugStretchHelixCurrentResult.nodes.size()
            << std::endl;

        return false;
    }

    if (!debugStretchHelixCurrentResult.isComplete())
    {
        std::cout
            << "[STRETCH HELIX CURRENT REBUILD]"
            << " accepted=0"
            << " reason=IntegrationIncomplete"
            << " nodes="
            << debugStretchHelixCurrentResult.nodes.size()
            << " requestedLength="
            << debugStretchHelixCurrentResult.requestedArcLength
            << " integratedLength="
            << debugStretchHelixCurrentResult.integratedArcLength
            << std::endl;

        return false;
    }

    std::cout
        << "[STRETCH HELIX CURRENT REBUILD]"
        << " accepted=1"
        << " nodes="
        << debugStretchHelixCurrentResult.nodes.size()
        << " wrappedLength="
        << state.wrappedLength
        << " frontS="
        << state.contactFrontS
        << std::endl;

    return true;




}

void AppController::
advanceDebugStretchHelixWrapping(
    double deltaWrappedLength)
{
    if (!debugStretchHelixWrappingState.valid)
        return;

    if (!std::isfinite(deltaWrappedLength)
        || deltaWrappedLength <= 0.0)
    {
        return;
    }

    const double totalLength =
        debugStretchHelixWrappingInput.pipeArcLength;

    debugStretchHelixWrappingState.wrappedLength =
        std::min(
            totalLength,
            debugStretchHelixWrappingState.wrappedLength
            + deltaWrappedLength
        );

    debugStretchHelixWrappingState.contactFrontS =
        debugStretchHelixWrappingState.wrappedLength;

    debugStretchHelixWrappingState.progress =
        debugStretchHelixWrappingState.wrappedLength
        / totalLength;

    debugStretchHelixWrappingState.complete =
        debugStretchHelixWrappingState.wrappedLength
        >= totalLength - 1e-12;

    rebuildDebugStretchHelixCurrentGeometry();

    std::cout
        << "[STRETCH HELIX WRAP STEP]"
        << " wrappedLength="
        << debugStretchHelixWrappingState.wrappedLength
        << " frontS="
        << debugStretchHelixWrappingState.contactFrontS
        << " progress="
        << debugStretchHelixWrappingState.progress
        << " complete="
        << debugStretchHelixWrappingState.complete
        << " geometryValid="
        << debugStretchHelixCurrentResult.valid
        << std::endl;

    const bool contactGeometryValid =
        rebuildDebugStretchHelixContactGeometry();
    std::cout
        << "[STRETCH HELIX WRAP STEP]"
        << " wrappedLength="
        << debugStretchHelixWrappingState.wrappedLength
        << " frontS="
        << debugStretchHelixWrappingState.contactFrontS
        << " progress="
        << debugStretchHelixWrappingState.progress
        << " complete="
        << debugStretchHelixWrappingState.complete
        << " profileGeometryValid="
        << debugStretchHelixCurrentResult.valid
        << " contactGeometryValid="
        << contactGeometryValid
        << std::endl;


}

void AppController::
resetDebugStretchHelixWrapping()
{
    debugStretchHelixProcess.reset();

    const StretchHelixWrappingState& state =
        debugStretchHelixProcess.getState();

    std::cout
        << "[STRETCH HELIX WRAP RESET]"
        << " wrappedLength="
        << state.wrappedLength
        << " frontS="
        << state.contactFrontS
        << " time="
        << state.elapsedTime
        << std::endl;
}

const SpatialCurveIntegrationResult&
AppController::
  getDebugStretchHelixReferenceResult() const
{
    return
        debugStretchHelixProcess
        .getReferenceResult();
}

void AppController::
debugTestStretchHelixProcessAcceptance()
{
    if (!debugStretchHelixProcess.isValid())
    {
        std::cout
            << "[STRETCH HELIX PROCESS ACCEPTANCE]"
            << " accepted=0"
            << " reason=InvalidProcess"
            << std::endl;

        return;
    }

    // Start from a known initial condition.
    debugStretchHelixProcess.reset();

    const StretchHelixWrappingInput& input =
        debugStretchHelixProcess.getInput();

    const StretchHelixWrappingKinematics& kinematics =
        debugStretchHelixProcess.getKinematics();

    // Controlled one-second test.
    constexpr double TEST_DT =
        1.0;

    debugStretchHelixProcess.advanceTime(
        TEST_DT
    );

    const StretchHelixWrappingState& state =
        debugStretchHelixProcess.getState();

    const double expectedLength =
        std::min(
            input.pipeArcLength,
            kinematics.centerlineSpeed
            * TEST_DT
        );

    constexpr double tolerance =
        1e-9;

    const bool lengthAccepted =
        std::abs(
            state.wrappedLength
            - expectedLength
        ) <= tolerance;

    const bool nodeCountAccepted =
        debugStretchHelixProcess
        .getCurrentNodes()
        .size()
        ==
        debugStretchHelixProcess
        .getReferenceResult()
        .nodes.size();

    const bool accepted =
        debugStretchHelixProcess.isValid()
        && lengthAccepted
        && nodeCountAccepted;

    std::cout
        << "[STRETCH HELIX PROCESS ACCEPTANCE]"
        << " wrappedLength="
        << state.wrappedLength
        << " expectedLength="
        << expectedLength
        << " currentNodes="
        << debugStretchHelixProcess
        .getCurrentNodes()
        .size()
        << " referenceNodes="
        << debugStretchHelixProcess
        .getReferenceResult()
        .nodes.size()
        << " lengthAccepted="
        << lengthAccepted
        << " nodeCountAccepted="
        << nodeCountAccepted
        << " accepted="
        << accepted
        << std::endl;

    // Important:
    // Return the real debug process to its normal startup state.
    debugStretchHelixProcess.reset();
}




bool AppController::
rebuildDebugStretchHelixContactGeometry()
{
    debugStretchHelixContactGeometryNodes.clear();

    if (!debugStretchHelixReferenceResult.valid)
        return false;

    if (!debugStretchHelixReferenceResult.isComplete())
        return false;

    const std::vector<PipeNode>& referenceNodes =
        debugStretchHelixReferenceResult.nodes;

    if (referenceNodes.size() < 2)
        return false;

    const double totalLength =
        debugStretchHelixWrappingInput.pipeArcLength;

    if (
        !debugStretchHelixWrappingState.isValidForLength(
            totalLength
        )
        )
    {
        return false;
    }

    const double frontS =
        debugStretchHelixWrappingState.contactFrontS;

    const double normalizedFront =
        std::clamp(
            frontS / totalLength,
            0.0,
            1.0
        );

    const std::size_t lastIndex =
        referenceNodes.size() - 1;

    const std::size_t frontIndex =
        static_cast<std::size_t>(
            std::llround(
                normalizedFront
                * static_cast<double>(
                    lastIndex
                    )
            )
            );

    debugStretchHelixContactGeometryNodes.reserve(
        referenceNodes.size()
    );

    // =====================================================
    // COPY WRAPPED PART FROM THE REFERENCE HELIX
    // =====================================================

    for (std::size_t i = 0;
        i <= frontIndex;
        ++i)
    {
        debugStretchHelixContactGeometryNodes.push_back(
            referenceNodes[i]
        );
    }

    Vec3D tangent;

    if (frontIndex == 0)
    {
        tangent =
            referenceNodes[1].pos
            - referenceNodes[0].pos;
    }
    else if (
        frontIndex >= lastIndex
        )
    {
        tangent =
            referenceNodes[lastIndex].pos
            - referenceNodes[lastIndex - 1].pos;
    }
    else
    {
        tangent =
            referenceNodes[frontIndex + 1].pos
            - referenceNodes[frontIndex - 1].pos;
    }

    tangent =
        tangent.normalized();



    const Vec3D frontPosition =
        referenceNodes[frontIndex].pos;

    const double remainingLength =
        totalLength
        - frontS;

    const std::size_t remainingNodeCount =
        lastIndex
        - frontIndex;

    if (remainingNodeCount > 0)
    {
        for (std::size_t j = 1;
            j <= remainingNodeCount;
            ++j)
        {
            const double localFraction =
                static_cast<double>(j)
                / static_cast<double>(
                    remainingNodeCount
                    );

            const double localLength =
                remainingLength
                * localFraction;

            PipeNode node =
                referenceNodes[frontIndex];

            node.pos =
                frontPosition
                + tangent
                * localLength;

            debugStretchHelixContactGeometryNodes.push_back(
                node
            );
        }

        const bool accepted =
            debugStretchHelixContactGeometryNodes.size()
            == referenceNodes.size();

        std::cout
            << "[STRETCH HELIX CONTACT GEOMETRY]"
            << " frontS="
            << frontS
            << " frontIndex="
            << frontIndex
            << " wrappedNodes="
            << (
                frontIndex + 1
                )
            << " totalNodes="
            << debugStretchHelixContactGeometryNodes.size()
            << " accepted="
            << accepted
            << std::endl;

        return accepted;
    }
}

void AppController::
advanceDebugStretchHelixWrappingTime(
    double dt)
{
    if (!debugStretchHelixProcess.isValid())
        return;

    if (debugStretchHelixProcess.isComplete())
    {
        std::cout
            << "[STRETCH HELIX TIME STEP]"
            << " ignored=1"
            << " reason=AlreadyComplete"
            << std::endl;

        return;
    }

    debugStretchHelixProcess.advanceTime(
        dt
    );

    // Temporary I didn't see current orangegeometry
    const auto& currentNodes =
        debugStretchHelixProcess.getCurrentNodes();

    const auto& referenceNodes =
        debugStretchHelixProcess
        .getReferenceResult()
        .nodes;

    std::cout
        << "[STRETCH HELIX PROCESS GEOMETRY]"
        << " currentNodes="
        << currentNodes.size()
        << " referenceNodes="
        << referenceNodes.size()
        << std::endl;

    const StretchHelixWrappingState& state =
        debugStretchHelixProcess.getState();

    std::cout
        << "[STRETCH HELIX TIME STEP]"
        << " dt="
        << dt
        << " elapsedTime="
        << state.elapsedTime
        << " wrappedLength="
        << state.wrappedLength
        << " frontS="
        << state.contactFrontS
        << " progress="
        << state.progress
        << " complete="
        << state.complete
        << " geometryValid="
        << debugStretchHelixProcess.isValid()
        << std::endl;
}


bool AppController::
isDebugStretchHelixWrappingComplete() const
{
    return
        debugStretchHelixProcess.isComplete();
}


bool AppController::
setDebugStretchHelixRotationSpeed(
    double speed)
{
    return
        debugStretchHelixProcess
        .setRotationSpeed(
            speed
        );
}


bool AppController::
setDebugStretchHelixAxialSpeed(
    double speed)
{
    return
        debugStretchHelixProcess
        .setAxialSpeed(
            speed
        );
}




void AppController::
debugTestStretchHelixWrappingTimeProgression()
{
    // =====================================================
    // H6.12 — TIME-DRIVEN WRAPPING ACCEPTANCE TEST
    //
    // This test verifies:
    //
    //     wrappedLength =
    //         centerlineSpeed * dt
    //
    // for one controlled time step.
    //
    // It does NOT modify the real playback state.
    // =====================================================

    const StretchHelixWrappingInput input =
        buildTestStretchHelixWrappingInput();

    if (!input.isValid())
    {
        std::cout
            << "[STRETCH HELIX TIME ACCEPTANCE]"
            << " accepted=0"
            << " reason=InvalidInput"
            << std::endl;

        return;
    }

    const StretchHelixWrappingKinematics kinematics =
        StretchHelixWrappingKinematicsBuilder::build(
            input
        );

    if (!kinematics.valid)
    {
        std::cout
            << "[STRETCH HELIX TIME ACCEPTANCE]"
            << " accepted=0"
            << " reason=InvalidKinematics"
            << std::endl;

        return;
    }

    StretchHelixWrappingState state =
        StretchHelixWrappingStateBuilder::buildInitial(
            input
        );

    if (!state.isValidForLength(
        input.pipeArcLength
    ))
    {
        std::cout
            << "[STRETCH HELIX TIME ACCEPTANCE]"
            << " accepted=0"
            << " reason=InvalidInitialState"
            << std::endl;

        return;
    }

    // =====================================================
    // CONTROLLED TEST STEP
    // =====================================================

    const double dt =
        1.0;

    const double expectedWrappedLength =
        std::min(
            input.pipeArcLength,
            kinematics.centerlineSpeed * dt
        );

    StretchHelixWrappingStateAdvancer::advance(
        state,
        dt,
        input,
        kinematics
    );

    // =====================================================
    // ACCEPTANCE
    // =====================================================

    constexpr double tolerance =
        1e-9;

    const bool timeAccepted =
        std::abs(
            state.elapsedTime - dt
        ) <= tolerance;

    const bool lengthAccepted =
        std::abs(
            state.wrappedLength
            - expectedWrappedLength
        ) <= tolerance;

    const bool frontAccepted =
        std::abs(
            state.contactFrontS
            - state.wrappedLength
        ) <= tolerance;

    const double expectedProgress =
        expectedWrappedLength
        / input.pipeArcLength;

    const bool progressAccepted =
        std::abs(
            state.progress
            - expectedProgress
        ) <= tolerance;

    const bool accepted =
        timeAccepted
        && lengthAccepted
        && frontAccepted
        && progressAccepted;

    std::cout
        << "[STRETCH HELIX TIME ACCEPTANCE]"
        << " dt="
        << dt
        << " centerlineSpeed="
        << kinematics.centerlineSpeed
        << " expectedLength="
        << expectedWrappedLength
        << " actualLength="
        << state.wrappedLength
        << " elapsedTime="
        << state.elapsedTime
        << " progress="
        << state.progress
        << " timeAccepted="
        << timeAccepted
        << " lengthAccepted="
        << lengthAccepted
        << " frontAccepted="
        << frontAccepted
        << " progressAccepted="
        << progressAccepted
        << " accepted="
        << accepted
        << std::endl;
}

double AppController::
getDebugStretchHelixRotationSpeed() const
{
    return
        debugStretchHelixProcess
        .getInput()
        .rotationSpeed;
}


double AppController::
getDebugStretchHelixAxialSpeed() const
{
    return
        debugStretchHelixProcess
        .getInput()
        .axialSpeed;
}

const std::vector<PipeNode>&
AppController::
getDebugStretchHelixContactGeometryNodes() const
{
    return
        debugStretchHelixProcess
        .getCurrentNodes();
}