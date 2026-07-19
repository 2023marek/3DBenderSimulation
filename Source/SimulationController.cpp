#include "Core/SimulationController.h"
#include <iostream>
#include <cmath>
#include "Core/Forming/DeformableRegionSelection.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

SimulationController::SimulationController()
    : playing(false),
    paused(false),
    speed(40.0),
    accumulatedDistance(0.0),
    accumulatedAngle(0.0),
    pipeSystem(0.5)
{
    std::cout << "SimulationController initialized\n";
}



void SimulationController::loadProgram(const std::vector<Operation>& ops)
{
    // ===================================================================
    // Store complete program
    // ===================================================================
    loadedOperations = ops;

    // ===================================================================
    // Reset runtime state for new program
    // ===================================================================
    operationQueue.load(ops);

    accumulatedDistance = 0.0;
    accumulatedAngle = 0.0;
    accumulatedRotation = 0.0;

    playing = false;
    paused = false;

    // ===================================================================
    // Reset systems
    // ===================================================================
    pipeSystem.reset();
    machineSystem.reset();

    lastDeformableRegionSelection.clear();

    // Preserve selected rotation kinematic mode
    pipe().setRotationKinematicMode(rotationKinematicMode);

    machineSystem
        .getRuntimeState()
        .rotationMode = rotationKinematicMode;

    // ===================================================================
    // Give full program to CAD / GeometricPipeModel
    // ===================================================================
    pipeSystem.setProgram(loadedOperations);

    const auto& cadNodes =
        pipeSystem.cadPipe().getNodes();

    std::cout << "[CAD TEST] GeometricPipeModel nodes="
        << cadNodes.size()
        << std::endl;

    std::cout << "[SimulationController] Program loaded: "
        << ops.size()
        << " operations\n";

    std::cout << "[DEBUG] loadedOperations now has: "
        << loadedOperations.size()
        << " ops\n";
}


void SimulationController::reset()
{
    operationQueue.reset();

    accumulatedDistance = 0.0;
    accumulatedAngle = 0.0;
    accumulatedRotation = 0.0;

    playing = false;
    paused = false;

    lastDeformableRegionSelection.clear();

    pipeSystem.reset();
    machineSystem.reset();

    pipe().setRotationKinematicMode(
        rotationKinematicMode
    );

    pipeSystem.setProgram(
        loadedOperations
    );

    std::cout
        << "[SimulationController] reset\n";
}
void SimulationController::play()
{
    std::cout << "[PLAY CALLED]\n";
    std::cout << "[PLAY CALLED] this=" << this << "\n";

    if (operationQueue.isComplete())
    {
        std::cout << "[PLAY BLOCKED] program complete\n";
        return;
    }

    std::cout << "[PLAY ACCEPTED]\n";

    playing = true;
    paused = false;

    machine().setStatus(MachineRuntimeState::Status::RUNNING);

    std::cout << "[PLAY STATE SET]\n";
    std::cout << "Simulation PLAYING (speed: "
        << speed
        << " mm/s)\n";
}

void SimulationController::pause()
{
    playing = false;
    paused = true;

    machine().setStatus(MachineRuntimeState::Status::PAUSED);

    std::cout << "Simulation PAUSED\n";
}

void SimulationController::step()
{
    if (operationQueue.isComplete())
    {
        std::cout << "? Simulation complete\n";
        return;
    }

    // Execute a small step (0.5 for FEED, 1° for BEND)
    const Operation* op = operationQueue.getCurrent();
    if (!op) return;

    if (op->type == Operation::FEED)
    {
        executeFeed(0.5);  // 0.5mm step
    }
    else if (op->type == Operation::BEND)
    {
        double stepAngle = (PI / 180.0) * 1.0;  // 1 degree step
        executeBend(stepAngle);
    }

    std::cout << "??  Step executed\n";
}





void SimulationController::update(double deltaTime)
{
    // =====================================================
    // CAD PREVIEW MODE
    //
    // CADPreview displays the full ideal pipe from
    // GeometricPipeModel.
    //
    // It should NOT process manufacturing FEED/BEND/ROTATE.
    // =====================================================

    if (mode == SimulationMode::CADPreview)
    {
        updatePipeGeometryCAD();
        return;
    }

    // =====================================================
    // MANUFACTURING PLAYBACK MODE
    // =====================================================

    if (!playing || operationQueue.isComplete())
        return;

    machine().advanceTime(deltaTime);

    executeOperation(deltaTime);

    const Operation* op = operationQueue.getCurrent();

    if (op)
    {
        if (op->type == Operation::FEED)
        {
            if (accumulatedDistance >= op->length)
            {
                accumulatedDistance = 0.0;
                advanceToNextOperation();
            }
        }
        else if (op->type == Operation::BEND)
        {
            if (accumulatedAngle >= op->angle)
            {
                accumulatedAngle = 0.0;
                advanceToNextOperation();
            }
        }
        else if (op->type == Operation::ROTATE)
        {
            if (accumulatedRotation >= op->angle)
            {
                accumulatedRotation = 0.0;
                advanceToNextOperation();
            }
        }
    }

    updatePipeGeometryManufacturing();

    const auto& ms =
        machineSystem.getRuntimeState();

    std::cout << "[MACHINE STATE] feed="
        << ms.feedPosition
        << " rotDeg="
        << ms.rotationAngle * 180.0 / PI
        << " bendDeg="
        << ms.bendAngle * 180.0 / PI
        << " time="
        << ms.currentTime
        << " feeding="
        << ms.feeding
        << " rotating="
        << ms.rotating
        << " bending="
        << ms.bending
        << std::endl;
}

void SimulationController::executeOperation(double deltaTime)
{
    const Operation* op =
        operationQueue.getCurrent();

    if (!op)
        return;

    if (deltaTime <= 0.0)
        return;

    // =====================================================
    // FEED
    //
    // Linear operation.
    //
    // speed:
    //   mm / second
    //
    // distance:
    //   mm this frame
    // =====================================================

    if (op->type == Operation::FEED)
    {
        double distance =
            speed * deltaTime;

        executeFeed(distance);
    }

    // =====================================================
    // BEND
    //
    // Material moves through bend by arc length.
    //
    // arcDistance:
    //   mm this frame
    //
    // angleToBend:
    //   radians this frame
    //
    // relation:
    //   arcLength = R * angle
    //   angle = arcLength / R
    // =====================================================

    else if (op->type == Operation::BEND)
    {
        double arcDistance =
            speed * deltaTime;

        if (op->R > 0.0)
        {
            double angleToBend =
                arcDistance / op->R;

            executeBend(angleToBend);
        }
    }

    // =====================================================
    // ROTATE
    //
    // Angular operation.
    //
    // rotationSpeedRadPerSec:
    //   radians / second
    //
    // angleIncrement:
    //   radians this frame
    //
    // This is independent from feed speed.
    // =====================================================

    else if (op->type == Operation::ROTATE)
    {
        double angleIncrement =
            rotationSpeedRadPerSec * deltaTime;

        executeRotate(angleIncrement);
    }
}

void SimulationController::executeFeed(double distance)
{
    const Operation* op =
        operationQueue.getCurrent();

    if (!op || op->type != Operation::FEED)
        return;

    lastOperationStopReason =
        OperationStopReason::None;

    double remaining =
        op->length - accumulatedDistance;

    double toMove =
        std::min(distance, remaining);

    if (toMove <= 0.0)
        return;

    machine().beginFeed();

    double actualFeed =
        pipe().processFeed(
            toMove
        );

    if (actualFeed <= 0.0)
    {
        if (pipe().isIncomingStockExhausted())
        {
            lastOperationStopReason =
                OperationStopReason::IncomingStockExhausted;
        }
        if (debugOperationStop)
        {
            std::cout
                << "[OP STOP] IncomingStockExhausted"
                << std::endl;
        }

        machine().endFeed();

        accumulatedDistance =
            op->length;

        return;
    }

    machine().addFeed(
        actualFeed
    );

    accumulatedDistance +=
        actualFeed;

    if (accumulatedDistance > op->length)
    {
        accumulatedDistance = op->length;
    }

    if (accumulatedDistance >= op->length - 1e-9)
    {
        machine().endFeed();
    }
}
void SimulationController::executeBend(double angle)
{
    const Operation* op =
        operationQueue.getCurrent();

    if (!op || op->type != Operation::BEND)
        return;

    double remaining =
        op->angle - accumulatedAngle;

    double toBend =
        std::min(angle, remaining);

    if (toBend <= 0.0)
        return;

    machine().beginBend(
        op->R,
        op->bendDirection
    );

    pipe().processBend(
        op->R,
        op->angle,
        toBend,
        op->bendDirection
    );

    machine().addBendAngle(toBend);

    accumulatedAngle += toBend;

    if (accumulatedAngle > op->angle)
    {
        accumulatedAngle = op->angle;
    }

    if (accumulatedAngle >= op->angle - 1e-9)
    {
        machine().endBend();
    }
}


void SimulationController::advanceToNextOperation()
{
    const Operation* currentOp =
        operationQueue.getCurrent();

    if (currentOp)
    {
        std::cout << "Operation complete: ";
        currentOp->print();
    }

    operationQueue.nextOperation();

    if (operationQueue.isComplete())
    {
        playing = false;
        paused = false;

        machine().setStatus(
            MachineRuntimeState::Status::COMPLETE
        );

        debugValidateFirstAdditionalPassRegionLength();

        debugSelectFirstAdditionalPassRegion(); 

        std::cout << "Program COMPLETED!\n";
    }
}


double SimulationController::getCurrentOperationProgress() const
{
    const Operation* op = operationQueue.getCurrent();

    // No current operation (queue complete or empty)
    if (!op)
        return 0.0;

    // FEED operation: track distance progress
    if (op->type == Operation::FEED)
    {
        if (op->length > 0.0)
        {
            // Progress: accumulated distance / total distance
            return std::min(1.0, accumulatedDistance / op->length);
        }
        return 1.0;  // Zero-length feed is "complete"
    }

    // BEND operation: track angle progress
    if (op->type == Operation::BEND)
    {
        if (op->angle > 0.0)
        {
            // Progress: accumulated angle / total angle
            return std::min(1.0, accumulatedAngle / op->angle);
        }
        return 1.0;  // Zero-angle bend is "complete"
    }
    // In getCurrentOperationProgress(), add before final return:

    // ROTATE operation: track angle progress
    if (op->type == Operation::ROTATE)
    {
        if (op->angle > 0.0)
        {
            // Progress: accumulated rotation / total angle
            return std::min(1.0, accumulatedRotation / op->angle);
        }
        return 1.0;  // Zero-angle rotate is "complete"
    }
    return 0.0;  // Unknown operation type
}

double SimulationController::getOverallProgress() const
{
    return operationQueue.getProgress();
}

void SimulationController::updatePipeGeometryCAD()
{
    // =====================================================
    // CAD PREVIEW MODE
    //
    // Uses GeometricPipeModel.
    // Does not touch ManufacturingPipeSimulator.
    // Does not touch PipeAxis3D legacy manufacturing axis.
    // =====================================================

    const auto& cadNodes =
        pipeSystem.cadPipe().getNodes();

    std::cout << "[CAD PREVIEW] GeometricPipeModel nodes: "
        << cadNodes.size()
        << std::endl;
}

//===========================================
// UPDATE PIPE GEOMETRY - MANUFACTURING 
// ===========================================

void SimulationController::updatePipeGeometryManufacturing()
{
    pipe().reconstructVisiblePipe();

    std::cout << "[MFG GEOM AFTER] nodes: "
        << pipe().getNodes().size()
        << std::endl;
}
// =========================================================================
// ROTATION EXECUTION - PHASE 3B
// =========================================================================
//
// CONCEPT: Twist around longitudinal axis
//
// Visual Explanation:
//
//   Looking down the pipe axis (from the front):
//
//   Before ROTATE:       During ROTATE:       After ROTATE 90°:
//
//      N ?                                           ? B
//      |                  (frame spinning)          |
//    B ? ---- T                                  ---- T
//      |                                        |
//      ?                                        ?
//                                               N
//
//   The Normal (N) and Binormal (B) vectors rotate
//   around the Tangent (T) axis, creating twist
//
// EXECUTION FLOW:
//
//   angleIncrement (radians this frame)
//            ?
//   Check if current op is ROTATE
//            ?
//   Calculate remaining = operation.angle - accumulatedRotation
//            ?
//   Take minimum of angleIncrement and remaining
//            ?
//   Apply rotation to MachineState
//            ?
//   Update accumulatedRotation
//            ?
//   Done! Frame is twisted.
//
// =========================================================================

void SimulationController::executeRotate(double angleIncrement)
{
    const Operation* op =
        operationQueue.getCurrent();

    if (!op || op->type != Operation::ROTATE)
        return;

    double remaining =
        op->angle - accumulatedRotation;

    double toRotate =
        std::min(angleIncrement, remaining);

    if (toRotate <= 0.0)
        return;

    // =====================================================
    // ROTATION SIGN
    //
    // accumulatedRotation stays positive.
    // signedRotation is used for actual geometry/machine motion.
    // =====================================================

    double signedRotation =
        toRotate * rotationDirectionSign(op->rotationDirection);

    machine().beginRotate();

    pipe().processRotate(signedRotation);
    machine().addRotation(signedRotation);

    accumulatedRotation += toRotate;

    if (accumulatedRotation > op->angle)
    {
        accumulatedRotation = op->angle;
    }

    if (accumulatedRotation >= op->angle - 1e-9)
    {
        machine().endRotate();
    }

    std::cout << "[ROTATE] stepDeg="
        << signedRotation * 180.0 / PI
        << " accumulatedDeg="
        << accumulatedRotation * 180.0 / PI
        << " targetDeg="
        << op->angle * 180.0 / PI
        << " dir="
        << rotationDirectionToString(op->rotationDirection)
        << std::endl;
}



//Helpers for refactoring



    // =====================================================
    // Helpers for PipeSystem refactor
    // =====================================================

   
    

OperationQueue& SimulationController::getQueue()
{
    return operationQueue;
}

const OperationQueue& SimulationController::getQueue() const
{
    return operationQueue;
}

PipeSystem& SimulationController::getPipeSystem()
{
    return pipeSystem;
}

const PipeSystem& SimulationController::getPipeSystem() const
{
    return pipeSystem;
}

ManufacturingPipeSimulator& SimulationController::pipe()
{
    return pipeSystem.manufacturingPipe();
}

const ManufacturingPipeSimulator& SimulationController::pipe() const
{
    return pipeSystem.manufacturingPipe();
}

ManufacturingHistory& SimulationController::getManufacturingHistory()
{
    return manufacturingHistory;
}

const ManufacturingHistory& SimulationController::getManufacturingHistory() const
{
    return manufacturingHistory;
}

SimulationController::OperationStopReason
SimulationController::getLastOperationStopReason() const
{
    return lastOperationStopReason;
}


void SimulationController::debugExecuteFirstAdditionalPassPlaceholder()
{
    if (manufacturingHistory.additionalPasses.empty())
    {
        return;
    }

    const AdditionalFormingPass& additionalPass =
        manufacturingHistory.additionalPasses.front();

    std::cout
        << "[ADDITIONAL REGION CHECK] start="
        << additionalPass.deformableRegion.startArcLength
        << " end="
        << additionalPass.deformableRegion.endArcLength
        << " valid="
        << additionalPass.deformableRegion.isValid()
        << std::endl;

    lastAdditionalPassExecutionResult =
        pipe().executeAdditionalFormingPass(
            additionalPass
        );

    std::cout
        << "[ADDITIONAL RESULT CHECK] "
        << additionalPassExecutionResultToString(
            lastAdditionalPassExecutionResult
        )
        << std::endl;
}




AdditionalPassExecutionResult
SimulationController::getLastAdditionalPassExecutionResult() const
{
    return lastAdditionalPassExecutionResult;
}


void SimulationController::debugSelectFirstAdditionalPassRegion()
{
    if (!debugDeformableRegionSelection)
        return;

    if (manufacturingHistory.additionalPasses.empty())
        return;

    const AdditionalFormingPass& additionalPass =
        manufacturingHistory.additionalPasses.front();

    lastDeformableRegionSelection =
        pipe().selectDeformableRegion(
            additionalPass.deformableRegion
        );

    const DeformableRegionSelection& selection =
        lastDeformableRegionSelection;

    std::cout
        << "[DEFORMABLE REGION SELECTION]"
        << " sourceArcLength="
        << selection.sourceArcLength
        << " beforeNodes="
        << selection.beforeNodes.size()
        << " selectedNodes="
        << selection.selectedNodes.size()
        << " afterNodes="
        << selection.afterNodes.size()
        << " valid="
        << selection.valid
        << std::endl;
}

void SimulationController::
debugValidateFirstAdditionalPassRegionLength()
{
    if (!debugDeformableRegionSelection)
        return;

    if (manufacturingHistory.additionalPasses.empty())
        return;

    const AdditionalFormingPass& additionalPass =
        manufacturingHistory.additionalPasses.front();

    double availableLength =
        pipe().getAvailablePrimaryOutputLength();

    double requiredEnd =
        additionalPass.deformableRegion.endArcLength;

    double missingLength =
        std::max(
            0.0,
            requiredEnd - availableLength
        );

    bool fits =
        requiredEnd <= availableLength + 1e-9;

    std::cout
        << "[DEFORMABLE REGION LENGTH CHECK]"
        << " availableLength="
        << availableLength
        << " requiredEnd="
        << requiredEnd
        << " missingLength="
        << missingLength
        << " fits="
        << fits
        << std::endl;
}



