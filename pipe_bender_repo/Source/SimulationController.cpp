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
    operationQueue.load(ops);
    machineState.reset();
    accumulatedDistance = 0.0;
    accumulatedAngle = 0.0;
    playing = false;
    paused = false;

    std::cout << "? Program loaded: " << ops.size() << " operations\n";
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

void SimulationController::reset()
{
    operationQueue.reset();
    machineState.reset();
    accumulatedDistance = 0.0;
    accumulatedAngle = 0.0;
    playing = false;
    paused = false;
    pipeGeometry = PipeAxis3D(5.0);

    std::cout << "?? Simulation RESET\n";
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

void SimulationController::updatePipeGeometry()
{
    // This will be updated when we integrate with main3D.cpp
    // For now, just maintain state
}

// =========================================================================
// PROGRESS TRACKING IMPLEMENTATIONS
// =========================================================================

double SimulationController::getCurrentOperationProgress() const
{
    // ??????????????????????????????????????????????
    // ? CURRENT OPERATION PROGRESS (0.0 to 1.0)    ?
    // ?                                            ?
    // ? If no operation available: return 0.0      ?
    // ? If FEED: progress = accDistance / opLength ?
    // ? If BEND: progress = accAngle / opAngle     ?
    // ??????????????????????????????????????????????

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
    // ????????????????????????????????????????????
    // ? OVERALL PROGRAM PROGRESS (0.0 to 1.0)    ?
    // ?                                          ?
    // ? Based on operation queue progress:       ?
    // ?   operationQueue.getProgress()           ?
    // ?                                          ?
    // ? Returns percentage of operations done    ?
    // ????????????????????????????????????????????

    // OperationQueue::getProgress() returns:
    //   currentIndex / totalOperations
    //
    // Example with 3 operations:
    //   Before start:  0 / 3 = 0.0   (0%)
    //   After op 1:    1 / 3 = 0.33  (33%)
    //   After op 2:    2 / 3 = 0.67  (67%)
    //   After op 3:    3 / 3 = 1.0   (100%)

    return operationQueue.getProgress();
}