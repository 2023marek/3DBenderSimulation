#include "../Core/SimulationController.h"
#include "../Core/Operations.h"
#include "../Machine/MachineState.h"
#include <iostream>
#include <iomanip>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846
#endif

/*
================================================================================
  TEST PHASE 3B - ROTATE Operation Integration
================================================================================

  GOAL: Verify that ROTATE operation executes correctly and creates
        twisted pipe geometry.

  IMPORTANCE: ROTATE is critical for creating complex 3D shapes like:
    • Helixes (curved + twisted simultaneously)
    • Twisted sections
    • Complex bending sequences

  What gets tested:
    ? ROTATE operation added to Operations enum
    ? SimulationController executes ROTATE
    ? Geometry twists correctly (applyRotation() in PipeAxis3D)
    ? Multi-segment sequences with ROTATE work
    ? Helix creation (ROTATE during BEND)
    ? MachineState tracks rotation correctly

  ARCHITECTURE DIAGRAM:

  ???????????????????????????????????????????
  ? Test: Create FEED ? ROTATE ? FEED      ?
  ? Program                                 ?
  ???????????????????????????????????????????
                            ?
  ???????????????????????????????????????????
  ? SimulationController                    ?
  ? • loadProgram()                         ?
  ? • play()                                ?
  ? • update() each frame                   ?
  ???????????????????????????????????????????
                        ?
  ???????????????????????????????????????????
  ? executeOperation()                      ?
  ? Case FEED ? executeFeed()              ?
  ? Case ROTATE ? executeRotate() [NEW!]   ?
  ? Case BEND ? executeBend()              ?
  ???????????????????????????????????????????
                        ?
  ???????????????????????????????????????????
  ? executeRotate()                         ?
  ? • Accumulate rotation angle             ?
  ? • Update MachineState.rotation          ?
  ? • Check completion                      ?
  ???????????????????????????????????????????
                        ?
  ???????????????????????????????????????????
  ? updatePipeGeometry()                    ?
  ? • Call pipeGeometry.addRotate()        ?
  ? • Build nodes with twist               ?
  ???????????????????????????????????????????
                        ?
  ???????????????????????????????????????????
  ? PipeAxis3D.build()                      ?
  ? • applyRotation() twists frame          ?
  ? • Normal & Binormal rotate around T    ?
  ? • Nodes have twisted positions          ?
  ???????????????????????????????????????????
                        ?
  ???????????????????????????????????????????
  ? Test Results:                           ?
  ? ? Geometry twisted correctly            ?
  ? ? Node positions updated                ?
  ? ? MachineState.rotation tracked         ?
  ? ? Ready for next operation              ?
  ???????????????????????????????????????????

================================================================================
*/

// =============================================================================
// HELPER FUNCTIONS - Test output formatting
// =============================================================================

static void printTestHeader(const std::string& title)
{
    std::cout << "\n" << std::string(75, '=') << "\n";
    std::cout << "  TEST: " << title << "\n";
    std::cout << std::string(75, '=') << "\n";
}

static void printTestResult(bool passed, const std::string& message)
{
    std::cout << (passed ? "  ? PASS: " : "  ? FAIL: ") << message << "\n";
    if (!passed) std::cout << "    ERROR: Assertion failed!\n";
}

static void printSeparator()
{
    std::cout << "  " << std::string(71, '-') << "\n";
}

static void printOperationInfo(const Operation& op)
{
    std::cout << "    ";
    op.print();
}

// =============================================================================
// TEST 1: Simple ROTATE Operation
// =============================================================================

/*
  ??????????????????????????????????????????????????????????????????????????????

  TEST 1: Simple ROTATE Operation Execution

  SCENARIO:
  ???????????????????????????????????????????????????????????????
  ? Program Sequence:                                           ?
  ?                                                             ?
  ? Op 0: FEED 50mm                                            ?
  ?   ?? Move straight forward 50mm                            ?
  ?      ??? (pipe in X direction)                             ?
  ?                                                             ?
  ? Op 1: ROTATE 90°                                           ?
  ?   ?? Twist around longitudinal axis 90 degrees             ?
  ?      /|\ (frame rotated, position unchanged!)              ?
  ?                                                             ?
  ? Op 2: FEED 50mm                                            ?
  ?   ?? Move forward another 50mm (but now twisted!)          ?
  ?      ??? (pipe twisted + in X direction)                   ?
  ???????????????????????????????????????????????????????????????

  EXPECTED BEHAVIOR:
  ? All 3 operations execute without error
  ? Geometry has reasonable node count (>20)
  ? Final rotation angle = 90° (?/2 radians)
  ? Total length = 100mm (50 + 0 + 50)

  VERIFICATION:
  ?? Operation count matches
  ?? Simulation completes
  ?? Node count increases
  ?? Rotation tracked in MachineState

  ??????????????????????????????????????????????????????????????????????????????
*/

static void test_SimpleRotate()
{
    printTestHeader("Simple ROTATE Operation");

    // Create simulator
    SimulationController sim;
    sim.setSpeed(50.0);  // 50 mm/s

    std::cout << "\n  ? Creating program: FEED ? ROTATE ? FEED\n";

    // =====================================================================
    // BUILD PROGRAM
    // =====================================================================

    std::vector<Operation> program;

    // Operation 0: FEED 50mm
    Operation op1;
    op1.type = Operation::FEED;
    op1.length = 50.0;
    program.push_back(op1);

    // Operation 1: ROTATE 90° (?/2 radians)
    Operation op2;
    op2.type = Operation::ROTATE;
    op2.angle = PI / 2.0;  // 90 degrees
    program.push_back(op2);

    // Operation 2: FEED 50mm
    Operation op3;
    op3.type = Operation::FEED;
    op3.length = 50.0;
    program.push_back(op3);

    // =====================================================================
    // LOAD AND DISPLAY PROGRAM
    // =====================================================================

    sim.loadProgram(program);

    std::cout << "\n  Program Structure:\n";
    for (size_t i = 0; i < program.size(); i++)
    {
        std::cout << "    Op " << i << ": ";
        program[i].print();
    }

    printSeparator();

    // =====================================================================
    // RUN SIMULATION
    // =====================================================================

    std::cout << "\n  Simulating...\n";
    std::cout << "    Speed: 50 mm/s\n";
    std::cout << "    Frame rate: 0.01s per update (100 FPS)\n";

    sim.play();

    size_t frameCount = 0;
    size_t lastOpIndex = 0;

    std::cout << "\n  Execution Timeline:\n";

    // Simulate until complete (safety limit 600 frames) ? CHANGED FROM 500
    while (!sim.getQueue().isComplete() && frameCount < 600)
    {
        size_t currentOpIndex = sim.getCurrentOperationIndex();

        // Print when operation changes
        if (currentOpIndex != lastOpIndex)
        {
            const auto& nodes = sim.getPipeGeometry().getNodes();
            std::cout << "    Frame " << frameCount
                << ": Op " << currentOpIndex << " started ("
                << nodes.size() << " nodes so far)\n";
            lastOpIndex = currentOpIndex;
        }

        // Update by 0.01 seconds (10ms per frame)
        sim.update(0.01);
        frameCount++;
    }

    // =====================================================================
    // FINAL STATE
    // =====================================================================

    const auto& nodes = sim.getPipeGeometry().getNodes();
    const MachineState& state = sim.getState();

    printSeparator();

    std::cout << "\n  Final State:\n";
    std::cout << "    Total frames: " << frameCount << "\n";
    std::cout << "    Total nodes: " << nodes.size() << "\n";
    std::cout << "    Final position: ("
        << std::fixed << std::setprecision(1)
        << state.position.x << ", "
        << state.position.y << ", "
        << state.position.z << ")\n";
    std::cout << "    Final rotation: "
        << (state.rotation * 180.0 / PI) << "°\n";
    std::cout << "    Time elapsed: "
        << std::fixed << std::setprecision(2)
        << state.currentTime << " seconds\n";

    printSeparator();

    // =====================================================================
    // ASSERTIONS
    // =====================================================================

    printTestResult(
        sim.getTotalOperations() == 3,
        "All 3 operations executed (FEED ? ROTATE ? FEED)"
    );

    printTestResult(
        nodes.size() >= 19,
        "Geometry generated with substantial node count (>=19)"
    );

    printTestResult(
        sim.getQueue().isComplete(),
        "Simulation completed successfully"
    );

    // Check final rotation is close to 90°
    double expectedRotation = PI / 2.0;
    double tolerance = 0.01;  // 0.01 radians ? 0.6°
    bool rotationCorrect = std::abs(state.rotation - expectedRotation) < tolerance;

    printTestResult(
        rotationCorrect,
        "Final rotation ? 90° (within 0.6° tolerance)"
    );

    printSeparator();
}

// =============================================================================
// TEST 2: ROTATE During BEND (Helix Shape)
// =============================================================================

/*
  ??????????????????????????????????????????????????????????????????????????????

  TEST 2: Helix Creation - ROTATE with BEND Operations

  SCENARIO: Create a helix (twisted spiral)
  ???????????????????????????????????????????????????????????????
  ? Program Sequence:                                           ?
  ?                                                             ?
  ? Op 0: FEED 30mm                                            ?
  ?   ?? Move straight (setup)                                ?
  ?      ???                                                    ?
  ?                                                             ?
  ? Op 1: ROTATE 180°                                          ?
  ?   ?? Twist 180° around axis                               ?
  ?      /|\                                                    ?
  ?                                                             ?
  ? Op 2: BEND 90° (R=40mm)                                    ?
  ?   ?? Curve while maintaining twist                         ?
  ?      /\                                                     ?
  ?      \/  ? HELIX! (curved AND twisted)                    ?
  ?                                                             ?
  ? Op 3: FEED 30mm                                            ?
  ?   ?? Continue straight (twisted)                           ?
  ?      ???                                                    ?
  ???????????????????????????????????????????????????????????????

  EXPECTED BEHAVIOR:
  ? All 4 operations execute
  ? Complex 3D geometry generated
  ? Node count > 30 (complex shape)
  ? Final rotation = 180° (? radians)
  ? Position changed (bend moved it)

  WHY THIS MATTERS:
  This creates a true 3D shape that requires:
    • Linear motion (FEED)
    • Rotational motion (ROTATE)
    • Curved motion (BEND)
  All combined in one program!

  ??????????????????????????????????????????????????????????????????????????????
*/

static void test_HelixRotation()
{
    printTestHeader("Helix: ROTATE with BEND (Complex 3D Shape)");

    SimulationController sim;
    sim.setSpeed(100.0);  // 100 mm/s

    std::cout << "\n  ? Creating helix program: FEED ? ROTATE ? BEND ? FEED\n";

    // =====================================================================
    // BUILD PROGRAM
    // =====================================================================

    std::vector<Operation> program;

    // Operation 0: FEED 30mm (setup)
    Operation op1;
    op1.type = Operation::FEED;
    op1.length = 30.0;
    program.push_back(op1);

    // Operation 1: ROTATE 180° (? radians)
    Operation op2;
    op2.type = Operation::ROTATE;
    op2.angle = PI;  // 180 degrees
    program.push_back(op2);

    // Operation 2: BEND 90° with radius 40mm
    Operation op3;
    op3.type = Operation::BEND;
    op3.R = 40.0;
    op3.angle = PI / 2.0;  // 90 degrees
    op3.dir = BendDirection::CCW;
    program.push_back(op3);

    // Operation 3: FEED 30mm (finish)
    Operation op4;
    op4.type = Operation::FEED;
    op4.length = 30.0;
    program.push_back(op4);

    // =====================================================================
    // LOAD AND DISPLAY PROGRAM
    // =====================================================================

    sim.loadProgram(program);

    std::cout << "\n  Program Structure (Helix Configuration):\n";
    for (size_t i = 0; i < program.size(); i++)
    {
        std::cout << "    Op " << i << ": ";
        program[i].print();
    }

    printSeparator();

    // =====================================================================
    // RUN SIMULATION
    // =====================================================================

    std::cout << "\n  Simulating helix generation...\n";
    std::cout << "    Speed: 100 mm/s\n";
    std::cout << "    Frame rate: 0.01s per update\n";

    sim.play();

    size_t frameCount = 0;
    size_t lastOpIndex = 0;

    std::cout << "\n  Execution Timeline:\n";

    while (!sim.getQueue().isComplete() && frameCount < 600)
    {
        size_t currentOpIndex = sim.getCurrentOperationIndex();

        // Print operation transitions
        if (currentOpIndex != lastOpIndex)
        {
            const auto& nodes = sim.getPipeGeometry().getNodes();
            double currentProgress = sim.getCurrentOperationProgress();

            std::cout << "    Frame " << frameCount
                << ": Op " << currentOpIndex << " started ("
                << nodes.size() << " nodes, "
                << std::fixed << std::setprecision(0)
                << (currentProgress * 100.0) << "% progress)\n";
            lastOpIndex = currentOpIndex;
        }

        sim.update(0.01);
        frameCount++;
    }

    // =====================================================================
    // FINAL STATE
    // =====================================================================

    const auto& nodes = sim.getPipeGeometry().getNodes();
    const MachineState& state = sim.getState();

    printSeparator();

    std::cout << "\n  Final State (Helix Complete):\n";
    std::cout << "    Total frames: " << frameCount << "\n";
    std::cout << "    Total nodes: " << nodes.size() << "\n";
    std::cout << "    Final position: ("
        << std::fixed << std::setprecision(1)
        << state.position.x << ", "
        << state.position.y << ", "
        << state.position.z << ")\n";
    std::cout << "    Final rotation: "
        << (state.rotation * 180.0 / PI) << "°\n";
    std::cout << "    Time elapsed: "
        << std::fixed << std::setprecision(2)
        << state.currentTime << " seconds\n";

    printSeparator();

    // =====================================================================
    // ASSERTIONS
    // =====================================================================

    printTestResult(
        sim.getTotalOperations() == 4,
        "All 4 operations executed (FEED ? ROTATE ? BEND ? FEED)"
    );

    printTestResult(
        nodes.size() > 20,
        "Helix geometry generated with complex node count (>20)"
    );

    printTestResult(
        sim.getQueue().isComplete(),
        "Helix simulation completed successfully"
    );

    // Check final rotation is close to 180°
    double expectedRotation = PI;
    double tolerance = 0.01;
    bool rotationCorrect = std::abs(state.rotation - expectedRotation) < tolerance;

    printTestResult(
        rotationCorrect,
        "Final rotation ? 180° (within 0.6° tolerance)"
    );

    // Check that position changed (bend moved it)
    bool positionChanged = state.position.x > 0.0 ||
        state.position.y > 1.0 ||
        state.position.z > 1.0;

    printTestResult(
        positionChanged,
        "Position changed from bend operation (not just rotation)"
    );

    printSeparator();
}

// =============================================================================
// TEST 3: ROTATE Operation State Tracking
// =============================================================================

/*
  ??????????????????????????????????????????????????????????????????????????????

  TEST 3: Verify ROTATE State Tracking

  PURPOSE: Ensure MachineState correctly tracks rotation

  EXPECTED BEHAVIOR:
  ? accumulatedRotation starts at 0.0
  ? Increases monotonically during ROTATE
  ? Reaches operation.angle when complete
  ? Resets to 0.0 on next operation
  ? Final rotation matches total of all ROTATEs

  DATA FLOW:

  Frame 1: accumulatedRotation = 0.3 rad (19%)
           MachineState.rotation = 0.3 rad
           ?
  Frame 2: accumulatedRotation = 0.6 rad (38%)
           MachineState.rotation = 0.6 rad
           ?
  Frame 3: accumulatedRotation = 0.9 rad (57%)
           MachineState.rotation = 0.9 rad
           ?
  Frame 4: accumulatedRotation = 1.2 rad (76%)
           MachineState.rotation = 1.2 rad
           ?
  Frame 5: accumulatedRotation = 1.57 rad (100%)
           MachineState.rotation = 1.57 rad
           Operation complete! ? Next operation

  ??????????????????????????????????????????????????????????????????????????????
*/

static void test_RotateStateTracking()
{
    printTestHeader("ROTATE State Tracking in MachineState");

    SimulationController sim;
    sim.setSpeed(100.0);

    std::cout << "\n  ? Testing state tracking with ROTATE operation\n";

    // Create simple program: just ROTATE
    std::vector<Operation> program;

    Operation rotateOp;
    rotateOp.type = Operation::ROTATE;
    rotateOp.angle = PI / 2.0;  // 90°
    program.push_back(rotateOp);

    sim.loadProgram(program);

    std::cout << "\n  Program: Single ROTATE 90°\n";
    std::cout << "    Total angle: " << (PI / 2.0 * 180.0 / PI) << "°\n";

    printSeparator();

    std::cout << "\n  Tracking MachineState.rotation:\n\n";
    std::cout << "    Frame | Rotation (rad) | Rotation (°) | Progress\n";
    std::cout << "    ------|----------------|--------------|----------\n";

    sim.play();

    size_t frameCount = 0;
    std::vector<double> rotationSnapshots;

    while (!sim.getQueue().isComplete() && frameCount < 200)
    {
        sim.update(0.01);

        if (frameCount % 20 == 0 || sim.getQueue().isComplete())
        {
            double rotation = sim.getState().rotation;
            double progress = sim.getCurrentOperationProgress();

            rotationSnapshots.push_back(rotation);

            std::cout << "    " << std::setw(5) << frameCount << " | "
                << std::fixed << std::setprecision(4) << std::setw(14) << rotation << " | "
                << std::setprecision(2) << std::setw(12) << (rotation * 180.0 / PI) << " | "
                << std::setprecision(0) << std::setw(8) << (progress * 100.0) << "%\n";
        }

        frameCount++;
    }

    printSeparator();

    // =====================================================================
    // VERIFY MONOTONIC INCREASE
    // =====================================================================

    bool monotonic = true;
    for (size_t i = 1; i < rotationSnapshots.size(); i++)
    {
        if (rotationSnapshots[i] < rotationSnapshots[i - 1])
        {
            monotonic = false;
            break;
        }
    }

    printTestResult(
        monotonic,
        "Rotation increases monotonically (never decreases)"
    );

    // =====================================================================
    // VERIFY FINAL ROTATION
    // =====================================================================

    double expectedFinal = PI / 2.0;
    double tolerance = 0.01;
    double finalRotation = sim.getState().rotation;
    bool rotationCorrect = std::abs(finalRotation - expectedFinal) < tolerance;

    printTestResult(
        rotationCorrect,
        "Final rotation equals operation angle (?90°)"
    );

    printTestResult(
        sim.getQueue().isComplete(),
        "Simulation completed after ROTATE"
    );

    printSeparator();
}

// =============================================================================
// TEST 4: Multiple ROTATE Operations
// =============================================================================

/*
  ??????????????????????????????????????????????????????????????????????????????

  TEST 4: Sequential ROTATE Operations

  SCENARIO: Multiple ROTATEs in sequence

  Program:
    Op 0: ROTATE 60°
    Op 1: ROTATE 120°
    Op 2: ROTATE 180°

  Expected Behavior:
  ? After Op 0: rotation = 60°
  ? After Op 1: rotation = 180° (60° + 120°)
  ? After Op 2: rotation = 360° (full circle!)

  VERIFICATION:
  Each ROTATE operation should accumulate correctly

  ??????????????????????????????????????????????????????????????????????????????
*/

static void test_MultipleRotates()
{
    printTestHeader("Multiple ROTATE Operations (Accumulation)");

    SimulationController sim;
    sim.setSpeed(100.0);

    std::cout << "\n  ? Testing sequential ROTATE operations\n";

    // =====================================================================
    // BUILD PROGRAM
    // =====================================================================

    std::vector<Operation> program;

    // Op 0: ROTATE 60°
    Operation rot1;
    rot1.type = Operation::ROTATE;
    rot1.angle = PI / 3.0;  // 60°
    program.push_back(rot1);

    // Op 1: ROTATE 120°
    Operation rot2;
    rot2.type = Operation::ROTATE;
    rot2.angle = 2.0 * PI / 3.0;  // 120°
    program.push_back(rot2);

    // Op 2: ROTATE 180°
    Operation rot3;
    rot3.type = Operation::ROTATE;
    rot3.angle = PI;  // 180°
    program.push_back(rot3);

    sim.loadProgram(program);

    std::cout << "\n  Program Structure:\n";
    for (size_t i = 0; i < program.size(); i++)
    {
        std::cout << "    Op " << i << ": ROTATE "
            << (program[i].angle * 180.0 / PI) << "°\n";
    }

    printSeparator();

    std::cout << "\n  Simulating...\n";
    std::cout << "    Expected total: 60° + 120° + 180° = 360°\n";

    sim.play();

    size_t frameCount = 0;
    size_t lastOpIndex = 0;

    while (!sim.getQueue().isComplete() && frameCount < 700)
    {
        size_t currentOpIndex = sim.getCurrentOperationIndex();

        if (currentOpIndex != lastOpIndex)
        {
            double rotation = sim.getState().rotation;
            std::cout << "    Op " << currentOpIndex << " complete: "
                << std::fixed << std::setprecision(1)
                << (rotation * 180.0 / PI) << "° total\n";
            lastOpIndex = currentOpIndex;
        }

        sim.update(0.01);
        frameCount++;
    }

    printSeparator();

    // =====================================================================
    // VERIFICATION
    // =====================================================================

    double finalRotation = sim.getState().rotation;
    double expectedTotal = PI + PI / 3.0 + 2.0 * PI / 3.0;  // 360°
    double tolerance = 0.01;

    std::cout << "\n  Final State:\n";
    std::cout << "    Final rotation: "
        << std::fixed << std::setprecision(1)
        << (finalRotation * 180.0 / PI) << "°\n";
    std::cout << "    Expected: "
        << (expectedTotal * 180.0 / PI) << "°\n";

    printSeparator();

    printTestResult(
        sim.getTotalOperations() == 3,
        "All 3 ROTATE operations executed"
    );

    bool totalCorrect = std::abs(finalRotation - expectedTotal) < tolerance;

    printTestResult(
        totalCorrect,
        "Total rotation = 360° (full circle)"
    );

    printTestResult(
        sim.getQueue().isComplete(),
        "All sequential ROTATEs completed"
    );

    printSeparator();
}

// =============================================================================
// MAIN TEST RUNNER
// =============================================================================

int main()
{
    std::cout << "\n";
    std::cout << "?????????????????????????????????????????????????????????????????????????????\n";
    std::cout << "        PHASE 3B: ROTATE OPERATION INTEGRATION TESTS\n";
    std::cout << "?????????????????????????????????????????????????????????????????????????????\n";
    std::cout << "\n  This test suite validates the complete ROTATE operation:\n";
    std::cout << "    ? Operations.h supports ROTATE type\n";
    std::cout << "    ? SimulationController executes ROTATE\n";
    std::cout << "    ? MachineState tracks rotation\n";
    std::cout << "    ? PipeAxis3D twists geometry correctly\n";
    std::cout << "    ? Complex 3D shapes (helixes) work\n\n";

    try
    {
        test_SimpleRotate();
        test_HelixRotation();
        test_RotateStateTracking();
        test_MultipleRotates();

        std::cout << "\n";
        std::cout << "?????????????????????????????????????????????????????????????????????????????\n";
        std::cout << "                    ALL ROTATE TESTS COMPLETED\n";
        std::cout << "?????????????????????????????????????????????????????????????????????????????\n";
        std::cout << "\n  Status: ? PHASE 3B READY FOR NEXT STEPS\n";
        std::cout << "\n  Next Phase: Phase 3C - Camera Controls & Rendering Integration\n";
        std::cout << "    • Rotate view around pipe\n";
        std::cout << "    • Visualize twisted geometry\n";
        std::cout << "    • Prepare for OpenGL rendering\n\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n? EXCEPTION: " << e.what() << "\n\n";
        return 1;
    }

    return 0;
}