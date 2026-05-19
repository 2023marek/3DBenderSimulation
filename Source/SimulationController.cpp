#include "Core/SimulationController.h"
#include <iostream>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846
#endif

SimulationController::SimulationController()
    : playing(false), paused(false), speed(40.0),
    accumulatedDistance(0.0), accumulatedAngle(0.0),
    pipeGeometry(0.5)
   


{
    std::cout << "? SimulationController initialized\n";
}


void SimulationController::loadProgram(const std::vector<Operation>& ops)
{
    // ===================================================================
    // KEY: Store operations BEFORE resetting anything
    // ===================================================================
    loadedOperations = ops;  // Store the COMPLETE program
    // Choose ONE mode here:

  
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
    pipeGeometry = PipeAxis3D(0.5);

    // ===================================================================
    // IMPORTANT: Do NOT clear loadedOperations on reset!
    // The operations are still needed for geometry rebuilding
    // ===================================================================

    std::cout << "?? Simulation RESET\n";
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
    machineState.setStatus(MachineState::Status::RUNNING);

    std::cout << "[PLAY STATE SET]\n";
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
    // Exit if not playing or already complete
    if (!playing || operationQueue.isComplete())
        return;

    // Calculate distance to move this frame
    // Example: speed=100mm/s, deltaTime=0.01s ? distance=1mm
    double distanceThisFrame = speed * deltaTime;

    // Execute current operation (FEED, BEND, or ROTATE)
    executeOperation(distanceThisFrame);

    // Update simulation time
    machineState.currentTime += deltaTime;
    std::cout << "dt: " << deltaTime << std::endl;
    // =====================================================================
    // CHECK IF CURRENT OPERATION IS COMPLETE
    // =====================================================================
    //
    // After executeOperation(), check if we've finished the current op
    // If yes, advance to next operation
    //
    const Operation* op = operationQueue.getCurrent();
    if (op)
    {
        if (op->type == Operation::FEED)
        {
            // FEED complete when accumulated distance >= operation length
            if (accumulatedDistance >= op->length)
            {
                accumulatedDistance = 0.0;
                advanceToNextOperation();
            }
        }
        else if (op->type == Operation::BEND)
        {
            // BEND complete when accumulated angle >= operation angle
            if (accumulatedAngle >= op->angle)
            {
                accumulatedAngle = 0.0;
                advanceToNextOperation();
            }
        }
        else if (op->type == Operation::ROTATE)  // ? NEW
        {
            // ROTATE complete when accumulated rotation >= operation angle
            if (accumulatedRotation >= op->angle)
            {
                accumulatedRotation = 0.0;  // Reset for next operation
                advanceToNextOperation();
            }
        }
    }

    // Update visualization geometry
    if (mode == SimulationMode::CADPreview)
    {
        updatePipeGeometryCAD();
    }
    else if (mode == SimulationMode::ManufacturingPlayback)
    {
        updatePipeGeometryManufacturing();
    }
}


void SimulationController::executeOperation(double distance)
{
    // Get current operation (FEED, BEND, or ROTATE)
    const Operation* op = operationQueue.getCurrent();
    if (!op) return;

    // =====================================================================
    // DISPATCH TO APPROPRIATE EXECUTOR BASED ON TYPE
    // =====================================================================

    if (op->type == Operation::FEED)
    {
        // ?????????????????????????????????????????????????????????????
        // FEED: Linear motion forward
        // ?????????????????????????????????????????????????????????????
        // distance = mm to move this frame
        executeFeed(distance);
    }
    else if (op->type == Operation::BEND)
    {
        // ?????????????????????????????????????????????????????????????
        // BEND: Arc motion with radius R
        // ?????????????????????????????????????????????????????????????
        // Arc length = radius * angle
        // Therefore: angle = arc_length / radius
        //
        if (op->R > 0.0)
        {
            double angleToRotate = distance / op->R;
            executeBend(angleToRotate);
        }
    }
    else if (op->type == Operation::ROTATE)
    {
        // ?????????????????????????????????????????????????????????????
        // ROTATE: Twist around longitudinal axis
        // ?????????????????????????????????????????????????????????????
        //
        // PARAMETER EXPLANATION:
        //   distance = speed (mm/s) * deltaTime (s) = mm traveled
        //   
        //   For ROTATE, we reinterpret this as rotation rate:
        //   angleIncrement (radians) = distance (mm) / 100.0
        //   
        //   This maps: 100mm/s speed ? ~1 radian/frame (at 0.01s)
        //
        // EXAMPLE EXECUTION:
        //   speed = 100 mm/s
        //   deltaTime = 0.01 s
        //   distance = 100 * 0.01 = 1.0 mm
        //   angleIncrement = 1.0 / 100.0 = 0.01 radians
        //   
        //   For ROTATE 90° (?/2 ? 1.57 radians):
        //   Frames needed: 1.57 / 0.01 ? 157 frames
        //   Time: 157 * 0.01 = 1.57 seconds ?
        //
        double angleIncrement = distance / 100.0;  // Convert mm to radians
        executeRotate(angleIncrement);
    }
}

void SimulationController::executeFeed(double distance)
{
    const Operation* op = operationQueue.getCurrent();
    if (!op || op->type != Operation::FEED) return;

    double remaining = op->length - accumulatedDistance;
    double toMove = std::min(distance, remaining);
    pipeGeometry.processFeed(toMove);
    //machineState.feedForward(toMove);
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

    if (!op || op->type != Operation::BEND)
        return;

    double remaining = op->angle - accumulatedAngle;
    double toBend = std::min(angle, remaining);

    pipeGeometry.processBend(
        op->R,
        op->angle,
        toBend
    );
    std::cout << "[EXECUTE BEND INPUT] R="
        << op->R
        << " angleRad="
        << op->angle
        << " angleDeg="
        << op->angle * 180.0 / PI
        << std::endl;
    accumulatedAngle += toBend;

    if (accumulatedAngle > op->angle)
    {
        accumulatedAngle = op->angle;
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
    // Create fresh geometry object for CAD preview mode
    pipeGeometry = PipeAxis3D(0.5);

    size_t currentIdx = operationQueue.getCurrentIndex();

    std::cout << "[CAD STEP] currentIdx: " << currentIdx << std::endl;
    std::cout << "[CAD STEP] accumulatedDistance: " << accumulatedDistance << std::endl;

    if (loadedOperations.empty())
    {
        std::cerr << "[CAD ERROR] loadedOperations is empty!\n";
        pipeGeometry.build();
        return;
    }

    // =====================================================
    // ADD ALL COMPLETED OPERATIONS
    // =====================================================

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
        else if (op.type == Operation::ROTATE)
        {
            pipeGeometry.addRotate(op.angle);
        }
    }

    // =====================================================
    // ADD PARTIAL CURRENT OPERATION
    // =====================================================

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
        else if (currentOp->type == Operation::ROTATE)
        {
            if (accumulatedRotation > 0.0)
            {
                pipeGeometry.addRotate(accumulatedRotation);
            }
        }
    }

    pipeGeometry.build();

    std::cout << "[CAD GEOM AFTER] nodes: "
        << pipeGeometry.getNodes().size()
        << std::endl;
}

//===========================================
// UPDATE PIPE GEOMETRY - MANUFACTURING 
// ===========================================

void SimulationController::updatePipeGeometryManufacturing()
{
    // =====================================================
    // MANUFACTURING MODE
    //
    // Do NOT recreate PipeAxis3D here.
    // Do NOT addFeed/addBend/addRotate here.
    // Do NOT rebuild CAD history here.
    //
    // PipeAxis3D already owns:
    // - incoming stock
    // - active zone
    // - frozen geometry
    //
    // We only ask it to assemble visible render nodes.
    // =====================================================

    pipeGeometry.reconstructVisiblePipe();

    std::cout << "[MFG GEOM AFTER] nodes: "
        << pipeGeometry.getNodes().size()
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
    // Step 1: Get current operation from queue
    const Operation* op = operationQueue.getCurrent();

    // Safety check: current operation MUST be ROTATE type
    if (!op || op->type != Operation::ROTATE)
    {
        // Not a ROTATE operation - exit silently
        return;
    }

    // Step 2: Calculate how much rotation is left to do
    //
    // If operation says ROTATE ?/2 (90°), and we've already done
    // ?/4 (45°), then remaining = ?/4 (45°)
    //
    double remaining = op->angle - accumulatedRotation;

    // Step 3: Take the smaller of:
    //   • angleIncrement (what we want to do this frame)
    //   • remaining (what's actually left in the operation)
    //
    // This ensures we don't overshoot
    //
    double toRotate = std::min(angleIncrement, remaining);

   
	// Step3a Inform PipeGeometry of rotation (for visual twist)
    // =====================================================
    // Apply rotation to geometric pipe model
    // =====================================================

    pipeGeometry.processRotate(toRotate);

 // Step 4: Apply rotation to machine state
    //
    // This tracks the total rotation of the entire pipe assembly
    // Used later for G-Code generation and visualization
    machineState.rotation += toRotate;

    // Step 5: Accumulate progress in this ROTATE operation
    //
    // When this equals operation.angle, the operation is complete
    //
    accumulatedRotation += toRotate;

    // Step 6: Safety clamp
    //
    // Prevent floating-point errors from exceeding target angle
    //
    if (accumulatedRotation > op->angle)
    {
        accumulatedRotation = op->angle;
    }
     
    // Done! Frame has been twisted by toRotate radians
}  

OperationQueue& SimulationController::getQueue()
{
    return operationQueue;   // ? correct
}

const OperationQueue& SimulationController::getQueue() const
{
    return operationQueue;   // ? correct
}
 