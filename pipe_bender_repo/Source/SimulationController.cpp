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
	loadedOperations = ops;  // Store for reference
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

// =========================================================================
// IMPLEMENTATION: updatePipeGeometry() - Option C (SMART HYBRID)
// =========================================================================
//
// THIS IS THE CRITICAL FUNCTION FOR REAL-TIME GEOMETRY!
//
// Challenge: We need to rebuild geometry based on:
//   1. All COMPLETED operations (full segments)
//   2. Current PARTIAL operation (partial segment)
//   3. NOT pending operations (not yet executed)
//
// Problem: OperationQueue stores operations privately, we can't iterate them
//
// Solution: Store operations locally in SimulationController!
//          (Requires adding member variable)
//
// For now: Simple approach that works:
//   - Rebuild geometry from scratch each call
//   - Only include executed operations
//
// =========================================================================

void SimulationController::updatePipeGeometry()
{
    // Create fresh geometry (resets all segments and nodes)
    pipeGeometry = PipeAxis3D(5.0);

    // Get current operation index
    size_t currentIdx = operationQueue.getCurrentIndex();

    // TEMPORARY WORKAROUND:
    // Since OperationQueue doesn't expose operations, we rebuild
    // geometry based on progress tracking variables.
    //
    // This works because:
    //   - accumulatedDistance tracks FEED progress
    //   - accumulatedAngle tracks BEND progress
    //   - currentIdx tells us which operation is executing
    //
    // For a complete solution, see FUTURE IMPROVEMENT below

    // For now, add a FEED operation representing current progress
    const Operation* op = operationQueue.getCurrent();

    if (op)
    {
        if (op->type == Operation::FEED)
        {
            // Add accumulated distance as the geometry
            if (accumulatedDistance > 0.0)
            {
                pipeGeometry.addFeed(accumulatedDistance);
            }
            else
            {
                // At least add start node
                pipeGeometry.addFeed(0.001);  // Tiny segment
            }
        }
        else if (op->type == Operation::BEND)
        {
            // Add accumulated bend as the geometry
            if (accumulatedAngle > 0.0)
            {
                pipeGeometry.addBend(op->R, accumulatedAngle);
            }
            else
            {
                // At least add start node
                pipeGeometry.addFeed(0.001);
            }
        }
    }
    else
    {
        // Queue empty or complete - add minimal geometry
        pipeGeometry.addFeed(0.001);
    }

    // Build the geometry nodes from segments
    pipeGeometry.build();

    //=====================================================================
    // FUTURE IMPROVEMENT: Store operations in SimulationController
    //=====================================================================
    //
    // To properly implement Option C, we need:
    //
    // Add to header:
    //   std::vector<Operation> loadedOperations;
    //
    // Modify loadProgram():
    //   loadedOperations = ops;  // Store locally
    //
    // Then in updatePipeGeometry():
    //   for (size_t i = 0; i < currentIdx; i++)
    //   {
    //       const auto& op = loadedOperations[i];
    //       if (op.type == Operation::FEED)
    //           pipeGeometry.addFeed(op.length);
    //       else if (op.type == Operation::BEND)
    //           pipeGeometry.addBend(op.R, op.angle);
    //   }
    //
    //=====================================================================
}

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