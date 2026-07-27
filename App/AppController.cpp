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
#include "Core/Geometry/ConstantCurvatureTorsionProfileBuilder.h"
#include "Core/Geometry/SpatialCurveIntegrator.h"
#include "Core/Geometry/SpatialCurveIntegrationResult.h"



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

    configureManufacturingDebug();
    configureControllerDebug();
    debugTestSpatialCurveIntegrator();
    debugTestSpatialHelixIntegrator();
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
