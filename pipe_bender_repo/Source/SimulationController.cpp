#include "../Core/SimulationController.h"
#include <iostream>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846
#endif

SimulationController::SimulationController()
    : playing(false), paused(false), speed(100.0),
    accumulatedDistance(0.0), accumulatedAngle(0.0),
    pipeGeometry(5.0)  // 5mm segment size
{
    std::cout << "? SimulationController initialized\n";
}


void SimulationController::loadProgram(const std::vector<Operation>& ops)
{
    // ===================================================================
    // KEY: Store operations BEFORE resetting anything
    // ===================================================================
    loadedOperations = ops;  // Store the COMPLETE program
    
    operationQueue.load(ops);
    machineState.reset();
    accumulatedDistance = 0.0;
    accumulatedAngle = 0.0;
    playing = false;
    paused = false;

    std::cout << "? Program loaded: " << ops.size() << " operations\n";
    std::cout << "  [DEBUG] loadedOperations now has: " << loadedOperations.size() << " ops\n";
}



void SimulationController::reset()
{
    operationQueue.reset();
    machineState.reset();
    accumulatedDistance = 0.0;
    accumulatedAngle = 0.0;
    playing = false;
    paused = false;
    pipeGeometry = PipeAxis3D(5.0);

    // ===================================================================
    // IMPORTANT: Do NOT clear loadedOperations on reset!
    // The operations are still needed for geometry rebuilding
    // ===================================================================

    std::cout << "?? Simulation RESET\n";
}

void SimulationController::play()
{
    if (operationQueue.isComplete())
    {
        std::cout << "??  Program already complete. Reset to play again.\n";
        return;
    }

    playing = true;
    paused = false;
    machineState.setStatus(MachineState::Status::RUNNING);
    std::cout << "??  Simulation PLAYING (speed: " << speed << " mm/s)\n";
}

void SimulationController::pause()
{
    playing = false;
    paused = true;
    machineState.setStatus(MachineState::Status::PAUSED);
    std::cout << "??  Simulation PAUSED\n";
}

void SimulationController::step()
{
    if (operationQueue.isComplete())
    {
        std::cout << "? Simulation complete\n";
        return;
    }

    // Execute a small step (5mm for FEED, 1° for BEND)
    const Operation* op = operationQueue.getCurrent();
    if (!op) return;

    if (op->type == Operation::FEED)
    {
        executeFeed(5.0);  // 5mm step
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
    if (!playing || operationQueue.isComplete())
        return;

    // Calculate distance to move this frame
    double distanceThisFrame = speed * deltaTime;  // mm

    // Execute current operation
    executeOperation(distanceThisFrame);

    // Update time
    machineState.currentTime += deltaTime;

    // Check if we need to advance to next operation
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
    }

    // Update visualization geometry
    updatePipeGeometry();
}

void SimulationController::executeOperation(double distance)
{
    const Operation* op = operationQueue.getCurrent();
    if (!op) return;

    if (op->type == Operation::FEED)
    {
        executeFeed(distance);
    }
    else if (op->type == Operation::BEND)
    {
        // Convert mm to angle (for constant speed over arc)
        // arc length = radius * angle
        // angle = arc length / radius
        if (op->R > 0.0)
        {
            double angleToRotate = distance / op->R;
            executeBend(angleToRotate);
        }
    }
}

void SimulationController::executeFeed(double distance)
{
    const Operation* op = operationQueue.getCurrent();
    if (!op || op->type != Operation::FEED) return;

    double remaining = op->length - accumulatedDistance;
    double toMove = std::min(distance, remaining);

    machineState.feedForward(toMove);
    accumulatedDistance += toMove;

    // Update progress (0.0 to 1.0)
    if (op->length > 0.0)
    {
        accumulatedDistance = std::min(accumulatedDistance, op->length);
    }
}

void SimulationController::executeBend(double angle)
{
    const Operation* op = operationQueue.getCurrent();
    if (!op || op->type != Operation::BEND) return;

    double remaining = op->angle - accumulatedAngle;
    double toRotate = std::min(angle, remaining);

    machineState.bendAngle(toRotate);
    accumulatedAngle += toRotate;

    // Update progress (0.0 to 1.0)
    if (op->angle > 0.0)
    {
        accumulatedAngle = std::min(accumulatedAngle, op->angle);
    }
}

void SimulationController::advanceToNextOperation()
{
    const Operation* currentOp = operationQueue.getCurrent();
    if (currentOp)
    {
        std::cout << "? Operation complete: ";
        currentOp->print();
    }

    operationQueue.nextOperation();

    if (operationQueue.isComplete())
    {
        playing = false;
        paused = false;
        machineState.setStatus(MachineState::Status::COMPLETED);
        std::cout << "?? Program COMPLETED!\n";
    }
}

// =========================================================================
// IMPLEMENTATION: updatePipeGeometry() - OPTION C (PROPER VERSION)
// =========================================================================
//
// STRATEGY: Accumulate ALL executed operations + current partial operation
//
// Execution Flow:
//
//   ????????????????????????????????????????????????
//   ? updatePipeGeometry()                         ?
//   ????????????????????????????????????????????????
//                    ?
//   ????????????????????????????????????????????????
//   ? Create fresh PipeAxis3D                      ?
//   ????????????????????????????????????????????????
//                    ?
//   ????????????????????????????????????????????????
//   ? Add ALL completed operations (full segments) ?
//   ? for i = 0 to currentIdx-1:                   ?
//   ?   Add loadedOperations[i] completely        ?
//   ????????????????????????????????????????????????
//                    ?
//   ????????????????????????????????????????????????
//   ? Add PARTIAL current operation                ?
//   ? if currentIdx < total:                       ?
//   ?   Add accumulated progress of current op    ?
//   ????????????????????????????????????????????????
//                    ?
//   ????????????????????????????????????????????????
//   ? Call pipeGeometry.build()                    ?
//   ? Generate all nodes from segments             ?
//   ????????????????????????????????????????????????
//                    ?
//   ????????????????????????????????????????????????
//   ? Result: Complete pipe up to current point    ?
//   ? Ready for rendering!                         ?
//   ????????????????????????????????????????????????
//
// =========================================================================


    // Step 5: Build geometry
    //
    // This converts segments into nodes (3D coordinates)
    // Ready for rendering!


// =========================================================================
// PROGRESS TRACKING IMPLEMENTATIONS
// =========================================================================

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

    return 0.0;  // Unknown operation type
}

double SimulationController::getOverallProgress() const
{
    return operationQueue.getProgress();
}
void SimulationController::updatePipeGeometry()
{
    pipeGeometry = PipeAxis3D(5.0);
    size_t currentIdx = operationQueue.getCurrentIndex();

    // ===================================================================
    // SAFETY CHECK: Verify loadedOperations is valid
    // ===================================================================
    if (loadedOperations.empty())
    {
        std::cerr << "  [ERROR] loadedOperations is empty!\n";
        std::cerr << "  [ERROR] currentIdx=" << currentIdx << "\n";
        pipeGeometry.build();
        return;
    }

    // Add completed operations
    for (size_t i = 0; i < currentIdx && i < loadedOperations.size(); i++)
    {
        const Operation& op = loadedOperations[i];

        if (op.type == Operation::FEED)
        {
            pipeGeometry.addFeed(op.length);
        }
        else if (op.type == Operation::BEND)
        {
            pipeGeometry.addBend(op.R, op.angle);
        }
    }

    // Add partial current operation
    const Operation* currentOp = operationQueue.getCurrent();
    if (currentOp)
    {
        if (currentOp->type == Operation::FEED)
        {
            if (accumulatedDistance > 0.0)
            {
                pipeGeometry.addFeed(accumulatedDistance);
            }
        }
        else if (currentOp->type == Operation::BEND)
        {
            if (accumulatedAngle > 0.0)
            {
                pipeGeometry.addBend(currentOp->R, accumulatedAngle);
            }
        }
    }

    // Build geometry
    pipeGeometry.build();
}