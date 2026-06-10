#include <sstream>
#include <iostream>
#include <vector>

#include "AppController.h"

#include "Core/Curve/PipeCurve.h"
#include "Core/Curve/PipeCurveSegment.h"
#include "Core/Sampling/PipeCurveSampler.h"
#include "Core/Forming/ManufacturingPass.h"
#include "Core/Forming/ManufacturingPlan.h"
#include "Core/Forming/HelixOperation.h"
#include "Core/Forming/HelixCurveBuilder.h"
#include "Core/Forming/HelixFormingPassBuilder.h"
#include "Core/Forming/ManufacturingPlanPreviewModel.h"
#include "Core/Forming/RotaryDrawPassBuilder.h"
#include "Core/Sampling/PipeCurveSampleQuery.h"

// =====================================
// CONSTRUCTOR
// =====================================
AppController::AppController()
{
    sim.getManufacturingPipe().setIncomingStockLength(
        500.0
    );
    std::vector<Operation> ops;

    Operation op1;
    op1.type = Operation::FEED;
    op1.length = 198.0;

    Operation op2;
    op2.type = Operation::BEND;
    op2.R = 20.0;
    op2.angle = PI / 2.0;
    op2.bendDirection = BendDirection::CCW;

    Operation op3;
    op3.type = Operation::ROTATE;
    op3.angle = PI / 2.0;
    op3.rotationDirection = RotationDirection::CCW;

    Operation op4;
    op4.type = Operation::FEED;
    op4.length = 110.0;

    ops.push_back(op1);
    ops.push_back(op2);
    ops.push_back(op3);
    ops.push_back(op4);

    // =====================================================
    // Load classic rotary draw operation queue.
    //
    // This is used by ManufacturingPlayback.
    // =====================================================

    sim.loadProgram(ops);

    // =====================================================
    // Build rotary draw pass from operation list.
    //
    // This creates the ideal curve representation of
    // the same FEED / BEND / ROTATE / FEED program.
    // =====================================================

    ManufacturingPass rotaryPass =
        RotaryDrawPassBuilder::buildPass(
            ops,
            "Rotary draw bending pass"
        );

    std::cout << "[ROTARY PASS TEST] ops="
        << rotaryPass.operations.size()
        << " curveSegments="
        << rotaryPass.outputCurve.size()
        << " completed="
        << rotaryPass.completed
        << std::endl;

    // =====================================================
    // Build helix forming pass.
    //
    // Current scope:
    // geometric helix + machine kinematics metadata.
    // No springback/material physics yet.
    // =====================================================

    HelixOperation helixPassOp;

    helixPassOp.inputMode =
        HelixOperation::InputMode::RadiusPitch;

    helixPassOp.length = 200.0;
    helixPassOp.helixRadius = 10.0;
    helixPassOp.pitch = 15.0;
    helixPassOp.feedSpeed = 40.0;

    ManufacturingPass helixPass =
        HelixFormingPassBuilder::buildPass(
            helixPassOp,
            "Heating element helix pass"
        );

    // =====================================================
    // Placement metadata.
    //
    // Phase 7O-1:
    // We only test locating arc length.
    // InsertAtArcLength is not applied yet.
    //
    // Phase 7O later:
    // This will control where the helix is inserted.
    // =====================================================

    helixPass.placement =
        PassPlacement::atArcLength(
            202.0
        );

    std::cout << "[PLACEMENT TEST] mode="
        << static_cast<int>(
            helixPass.placement.mode
            )
        << " arcLength="
        << helixPass.placement.arcLength
        << std::endl;

    // =====================================================
    // Build multi-pass plan.
    //
    // Current composition still appends curves.
    // InsertAtArcLength is stored as metadata only for now.
    // =====================================================

   

    PipeCurve rotaryCurve =
        rotaryPass.outputCurve;

    PipeCurveLocation loc =
        rotaryCurve.locateArcLength(
            202.0
        );

    std::cout << "[ROTARY CURVE LOCATION TEST] valid="
        << loc.valid
        << " segmentIndex="
        << loc.segmentIndex
        << " globalS="
        << loc.globalS
        << " localS="
        << loc.localS
        << " segmentStart="
        << loc.segmentStartS
        << " segmentEnd="
        << loc.segmentEndS
        << std::endl;

    PipeCurveSplitResult split =
        rotaryCurve.splitAtArcLength(
            202.0
        );

    std::cout << "[ROTARY CURVE SPLIT TEST] valid="
        << split.valid
        << " beforeSegments="
        << split.before.size()
        << " beforeLength="
        << split.before.totalLength()
        << " afterSegments="
        << split.after.size()
        << " afterLength="
        << split.after.totalLength()
        << std::endl;

    auto rotaryNodes =
        PipeCurveSampler::sample(
            rotaryCurve,
            0.5
        );

    auto frameQuery =
        PipeCurveSampleQuery::findFrameAtArcLength(
            rotaryNodes,
            202.0
        );

    std::cout << "[FRAME QUERY TEST] valid="
        << frameQuery.valid
        << " targetS="
        << frameQuery.targetS
        << " nearestS="
        << frameQuery.nearestS
        << " error="
        << frameQuery.error
        << " nodeIndex="
        << frameQuery.nodeIndex
        << " P=("
        << frameQuery.frame.P.x << ", "
        << frameQuery.frame.P.y << ", "
        << frameQuery.frame.P.z << ")"
        << std::endl;

 ManufacturingPlan multiPassPlan;

    multiPassPlan.addPass(
        rotaryPass
    );

    multiPassPlan.addPass(
        helixPass
    );



    // =====================================================
    // Phase 7O-1 curve-location test.
    //
    // Locate global arc length s = 80 mm inside the
    // composed curve.
    //
    // Expected:
    // s=80 is inside first FEED segment:
    // segmentIndex=0, localS=80, segmentStart=0, segmentEnd=120.
    // =====================================================

    PipeCurve combinedCurve =
        multiPassPlan.buildCombinedCurve();

    // =====================================================
    // Store complete multi-pass plan in the simulation.
    //
    // PlannedShapePreview renders this final composed shape.
    // ManufacturingPlayback ignores the helix pass for now
    // and uses only the operation queue.
    // =====================================================

    sim.getManufacturingPlanPreview().setPlan(
        multiPassPlan
    );

    // =====================================================
    // Choose active mode.
    //
    // ManufacturingPlayback:
    //      process simulation with incoming stock,
    //      positioned straight, active zone, frozen geometry.
    //
    // PlannedShapePreview:
    //      final composed curve preview:
    //      rotary pass + helix pass.
    // =====================================================

   //sim.setMode(
  //   SimulationController::SimulationMode::ManufacturingPlayback
   //);

     sim.setMode(
        SimulationController::SimulationMode::PlannedShapePreview
   );

    // sim.setMode(
  //     SimulationController::SimulationMode::CADPreview
    // );
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

    data.currentOpIndex = sim.getCurrentOperationIndex();
    data.totalOperations = sim.getTotalOperations();

    data.currentOpProgress = sim.getCurrentOperationProgress();
    data.overallProgress = sim.getOverallProgress();

    if (sim.getMode() == SimulationController::SimulationMode::ManufacturingPlayback)
    {
        data.nodeCount = sim.getManufacturingPipe().getNodes().size();
    }
    else if (sim.getMode() == SimulationController::SimulationMode::CADPreview)
    {
        data.nodeCount = sim.getCadPipeGeometry().getNodes().size();
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
    }
}