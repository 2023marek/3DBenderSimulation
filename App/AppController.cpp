#include <sstream>
#include <iostream>
#include <vector>
#include <cmath>

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

    constexpr double TEST_INTEGRATOR_ARC_LENGTH =
        100.0;

    constexpr double TEST_INTEGRATOR_CURVATURE =
        0.02;

    constexpr double TEST_INTEGRATOR_TORSION =
        0.0;

    constexpr double TEST_INTEGRATOR_SAMPLE_STEP =
        0.5;
    //Threshold initial
    constexpr double TEST_MAX_ENDPOINT_POSITION_ERROR =
        0.05; // mm

    constexpr double TEST_MAX_ENDPOINT_TANGENT_ERROR =
        1e-3;

    constexpr double TEST_MAX_INTEGRATED_LENGTH_ERROR =
        1e-6; // mm

    constexpr double TEST_MAX_RELATIVE_POSITION_ERROR =
        1e-3;
    // helix spatial test
    constexpr bool DEBUG_TEST_SPATIAL_HELIX_INTEGRATOR =
        true;

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
        0.01;

    constexpr double TEST_STRETCH_TARGET_TORSION =
        0.0;

    constexpr double TEST_STRETCH_AXIAL_STRAIN =
        0.02;

    constexpr double TEST_STRETCH_FEED_SPEED =
        40.0;

    constexpr double TEST_STRETCH_SAMPLE_STEP =
        0.5;

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

    StretchBendingProcessInput rejectedInput =
        buildTestStretchBendingProcessInput();

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
        << " evaluationStatus="
        << stretchBendingEvaluationStatusToString(
            rejectedEvaluation.status
        )
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
        0.25;

    // =====================================================
    // 2. EVALUATE MATERIAL / PROCESS FEASIBILITY
    //
    // Exactly one evaluator and one evaluation result belong
    // to this function. Previous compiler errors were caused
    // by declaring these variables a second time below.
    // =====================================================

    StretchBendingEvaluator evaluator;

    const StretchBendingEvaluationResult evaluation =
        evaluator.evaluate(
            input
        );
    
    if (!evaluation.valid)
    {
        // Clear every output owned by this debug scenario so
        // stale geometry/state cannot remain visible.
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
   
   