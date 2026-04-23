#include "../Core/SimulationController.h"
#include "../Core/Operations.h"
#include <iostream>
#include <cassert>
#include <iomanip>
#include <cmath>

// =========================================================================
// PI CONSTANT - Used for angle calculations (90° = PI/2 radians)
// =========================================================================
#ifndef PI
#define PI 3.14159265358979323846
#endif

/*
================================================================================
  TEST PHASE 2 - SimulationController Functionality Tests
================================================================================

  This test suite validates:
    ? Program loading
    ? Playback control (play, pause, step, reset)
    ? Operation execution (FEED and BEND)
    ? Progress tracking
    ? State management
    ? Edge cases and error conditions

  EXECUTION FLOW:

  Program Loaded
       ?
   [Play/Step]
       ?
   Execute FEED/BEND
       ?
   Check Progress
       ?
   Advance if Complete
       ?
   All Done? ? YES ? [Reset]
       ? NO
    Continue

================================================================================
*/

// =============================================================================
// HELPER FUNCTIONS (marked static - internal use only)
// =============================================================================

/// Print formatted test section header
/// @param title Name of the test
static void printTestHeader(const std::string& title)
{
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  TEST: " << title << "\n";
    std::cout << std::string(70, '=') << "\n";
}

/// Print test result with pass/fail indicator
/// @param passed Test result (true = pass, false = fail)
/// @param message Description of what was tested
static void printTestResult(bool passed, const std::string& message)
{
    std::cout << (passed ? "  ? PASS: " : "  ? FAIL: ") << message << "\n";
    if (!passed) std::cout << "    ERROR: Test assertion failed!\n";
}

/// Print separator line for test organization
static void printSeparator()
{
    std::cout << "  " << std::string(66, '-') << "\n";
}

// =============================================================================
// TEST 1: Program Loading
// =============================================================================

/// Test: Load operations into SimulationController
/// Validates:
///   • Correct number of operations loaded
///   • Initial progress = 0.0
///   • Initial state = IDLE
static void test_LoadProgram()
{
    printTestHeader("Program Loading");

    SimulationController sim;

    // Create test program
    //   FEED 100 mm
    //   BEND R=50mm, angle=90°
    //   FEED 80 mm
    //
    // ????????????????????????????????
    // ? Feed 100mm ? Bend 90° ? Feed ?
    // ?            (R=50)        80mm?
    // ????????????????????????????????

    std::vector<Operation> program;

    Operation op1;
    op1.type = Operation::FEED;
    op1.length = 100.0;
    program.push_back(op1);

    Operation op2;
    op2.type = Operation::BEND;
    op2.R = 50.0;
    op2.angle = PI / 2.0;  // 90 degrees
    op2.dir = BendDirection::CCW;
    program.push_back(op2);

    Operation op3;
    op3.type = Operation::FEED;
    op3.length = 80.0;
    program.push_back(op3);

    // Load into controller
    sim.loadProgram(program);

    // Verify
    printTestResult(
        sim.getTotalOperations() == 3,
        "Loaded 3 operations"
    );

    printTestResult(
        sim.getOverallProgress() == 0.0,
        "Progress starts at 0.0"
    );

    printTestResult(
        !sim.isPlaying() && !sim.isPaused(),
        "Initial state is IDLE (not playing, not paused)"
    );

    printTestResult(
        sim.getState().status == MachineState::Status::IDLE,
        "Machine status is IDLE"
    );

    printSeparator();
}

// =============================================================================
// TEST 2: Playback Control (Play/Pause/Reset)
// =============================================================================

/// Test: Playback state transitions
/// Validates:
///   • play() sets RUNNING state
///   • pause() sets PAUSED state
///   • Can resume after pause
///   • reset() returns to IDLE with 0% progress
static void test_PlaybackControl()
{
    printTestHeader("Playback Control (Play/Pause/Reset)");

    SimulationController sim;

    // Load simple program
    std::vector<Operation> program;
    Operation op;
    op.type = Operation::FEED;
    op.length = 100.0;
    program.push_back(op);

    sim.loadProgram(program);

    // TEST: Play
    std::cout << "\n  ? Testing Play...\n";
    sim.play();

    printTestResult(
        sim.isPlaying(),
        "isPlaying() returns true after play()"
    );

    printTestResult(
        sim.getState().status == MachineState::Status::RUNNING,
        "Machine status changed to RUNNING"
    );

    // TEST: Pause
    printSeparator();
    std::cout << "\n  ? Testing Pause...\n";
    sim.pause();

    printTestResult(
        sim.isPaused() && !sim.isPlaying(),
        "isPaused() returns true, isPlaying() returns false"
    );

    printTestResult(
        sim.getState().status == MachineState::Status::PAUSED,
        "Machine status changed to PAUSED"
    );

    // TEST: Resume (play after pause)
    printSeparator();
    std::cout << "\n  ? Testing Resume (play after pause)...\n";
    sim.play();

    printTestResult(
        sim.isPlaying() && !sim.isPaused(),
        "Can resume: isPlaying() true, isPaused() false"
    );

    // TEST: Reset
    printSeparator();
    std::cout << "\n  ? Testing Reset...\n";
    sim.reset();

    printTestResult(
        !sim.isPlaying() && !sim.isPaused(),
        "After reset: not playing, not paused"
    );

    printTestResult(
        sim.getOverallProgress() == 0.0,
        "Progress reset to 0.0"
    );

    printTestResult(
        sim.getState().status == MachineState::Status::IDLE,
        "Machine status reset to IDLE"
    );

    printSeparator();
}

// =============================================================================
// TEST 3: Step Execution
// =============================================================================

/// Test: Manual step-by-step execution
/// Validates:
///   • step() executes small increments (5mm/1°)
///   • Advancing to next operation when current completes
///   • Correct operation index tracking
static void test_StepExecution()
{
    printTestHeader("Step Execution (Manual Stepping)");

    SimulationController sim;

    // Load program
    std::vector<Operation> program;

    Operation op1;
    op1.type = Operation::FEED;
    op1.length = 100.0;
    program.push_back(op1);

    Operation op2;
    op2.type = Operation::BEND;
    op2.R = 50.0;
    op2.angle = PI / 2.0;
    op2.dir = BendDirection::CCW;
    program.push_back(op2);

    sim.loadProgram(program);

    // STEP 1: Execute first step (5mm of 100mm FEED)
    std::cout << "\n  ? Executing step 1 (5mm FEED)...\n";
    std::cout << "     Current operation: ";
    const Operation* op = sim.getQueue().getCurrent();
    if (op) op->print();

    sim.step();

    printTestResult(
        sim.getCurrentOperationIndex() == 0,
        "Still on operation 0 (not completed yet)"
    );

    // STEP 2: Execute multiple steps to complete first operation
    std::cout << "\n  ? Executing steps 2-20 (completing FEED)...\n";
    for (int i = 0; i < 20; i++)
        sim.step();

    // After 20 steps, we've done 20*5=100mm (complete)
    // Should move to operation 1
    printTestResult(
        sim.getCurrentOperationIndex() == 1,
        "Advanced to operation 1 (BEND) after completing FEED"
    );

    std::cout << "\n     Current operation: ";
    op = sim.getQueue().getCurrent();
    if (op) op->print();

    printTestResult(
        !sim.getQueue().isComplete(),
        "Queue is not complete (more operations remain)"
    );

    printSeparator();
}

// =============================================================================
// TEST 4: Continuous Update (Time-based)
// =============================================================================

/// Test: Time-based simulation update
/// Validates:
///   • Distance calculation (speed * deltaTime)
///   • Progress tracking between operations
///   • Automatic operation advancement on completion
///   • COMPLETED state when all operations done
static void test_ContinuousUpdate()
{
    printTestHeader("Continuous Update (Time-based Simulation)");

    SimulationController sim;
    sim.setSpeed(100.0);  // 100 mm/sec

    // Load simple program: 100mm FEED
    std::vector<Operation> program;
    Operation op;
    op.type = Operation::FEED;
    op.length = 100.0;
    program.push_back(op);

    sim.loadProgram(program);

    // STATE MACHINE:
    // ??????????????
    // ?   IDLE     ?
    // ??????????????
    //      ? loadProgram
    // ??????????????
    // ?  LOADED    ?
    // ??????????????
    //      ? play()
    // ??????????????
    // ?  RUNNING   ? ? update() called here
    // ??????????????
    //      ? operation complete
    // ??????????????
    // ? COMPLETED  ?
    // ??????????????

    sim.play();

    // Simulate 0.5 seconds at 100mm/sec = 50mm traveled
    std::cout << "\n  ? Updating with deltaTime=0.5s (speed=100mm/s)...\n";
    std::cout << "     Expected: 50mm traveled\n";

    sim.update(0.5);

    double expectedProgress = 50.0 / 100.0;  // 0.5
    printTestResult(
        sim.getCurrentOperationIndex() == 0,
        "Still on operation 0 (50mm of 100mm done)"
    );

    // Simulate another 0.6 seconds = 60mm, total 110mm (completes!)
    std::cout << "\n  ? Updating with deltaTime=0.6s (speed=100mm/s)...\n";
    std::cout << "     Expected: 60mm more (total 110mm ? operation complete)\n";

    sim.update(0.6);

    printTestResult(
        sim.getQueue().isComplete(),
        "Queue complete (all operations done)"
    );

    printTestResult(
        sim.getState().status == MachineState::Status::COMPLETED,
        "Machine status is COMPLETED"
    );

    printSeparator();
}

// =============================================================================
// TEST 5: Progress Tracking
// =============================================================================

/// Test: Overall progress across multiple operations
/// Validates:
///   • Progress increases as operations complete
///   • Progress = 1.0 when all done
///   • getOverallProgress() reflects operation queue completion
static void test_ProgressTracking()
{
    printTestHeader("Progress Tracking");

    SimulationController sim;
    sim.setSpeed(50.0);  // 50mm/sec

    // Load multi-operation program
    std::vector<Operation> program;

    Operation op1;
    op1.type = Operation::FEED;
    op1.length = 100.0;
    program.push_back(op1);

    Operation op2;
    op2.type = Operation::FEED;
    op2.length = 100.0;
    program.push_back(op2);

    Operation op3;
    op3.type = Operation::FEED;
    op3.length = 100.0;
    program.push_back(op3);

    sim.loadProgram(program);

    std::cout << "\n  Program Structure:\n";
    std::cout << "    Op0: FEED 100mm\n";
    std::cout << "    Op1: FEED 100mm\n";
    std::cout << "    Op2: FEED 100mm\n";
    std::cout << "    Total: 300mm\n";

    sim.play();

    // Progress through operations
    std::cout << "\n  ? After 1.0s (50mm):\n";
    sim.update(1.0);
    std::cout << "     Overall progress: " << (sim.getOverallProgress() * 100.0) << "%\n";

    printTestResult(
        sim.getOverallProgress() > 0.0 && sim.getOverallProgress() < 1.0,
        "Overall progress between 0% and 100%"
    );

    std::cout << "\n  ? After 4.0s total (200mm total):\n";
    sim.update(3.0);
    std::cout << "     Overall progress: " << (sim.getOverallProgress() * 100.0) << "%\n";

    std::cout << "\n  ? After 6.0s total (300mm total, COMPLETE):\n";
    sim.update(2.0);
    std::cout << "     Overall progress: " << (sim.getOverallProgress() * 100.0) << "%\n";

    printTestResult(
        sim.getOverallProgress() == 1.0,
        "Overall progress = 100% at completion"
    );

    printSeparator();
}

// =============================================================================
// TEST 6: Edge Cases
// =============================================================================

/// Test: Edge cases and error handling
/// Validates:
///   • Empty program behavior
///   • Multiple resets
///   • Preventing playback of completed programs
///   • Reset allows re-execution
static void test_EdgeCases()
{
    printTestHeader("Edge Cases & Error Handling");

    SimulationController sim;

    // TEST: Play with empty program
    std::cout << "\n  ? Testing play() with empty program...\n";
    sim.play();
    printTestResult(
        sim.getQueue().isComplete(),
        "Empty program is immediately complete"
    );

    // TEST: Multiple resets
    std::cout << "\n  ? Testing multiple resets...\n";
    std::vector<Operation> program;
    Operation op;
    op.type = Operation::FEED;
    op.length = 100.0;
    program.push_back(op);

    sim.loadProgram(program);
    sim.play();
    sim.update(0.5);
    sim.reset();
    sim.reset();  // Reset twice

    printTestResult(
        sim.getOverallProgress() == 0.0,
        "Multiple resets work correctly"
    );

    // TEST: Play after completion, then reset
    std::cout << "\n  ? Testing play after completion...\n";
    sim.loadProgram(program);
    sim.play();
    sim.update(2.0);  // Complete the program

    sim.play();  // Try to play again
    printTestResult(
        !sim.isPlaying(),
        "Cannot play completed program without reset"
    );

    sim.reset();
    sim.play();
    printTestResult(
        sim.isPlaying(),
        "Can play after reset"
    );

    printSeparator();
}

// =============================================================================
// MAIN TEST RUNNER
// =============================================================================

int main()
{
    std::cout << "\n";
    std::cout << "??????????????????????????????????????????????????????????????????????????\n";
    std::cout << "?                   SIMULATION CONTROLLER - TEST PHASE 2                 ?\n";
    std::cout << "?                                                                        ?\n";
    std::cout << "?  Testing:                                                              ?\n";
    std::cout << "?    • Program loading and initialization                                ?\n";
    std::cout << "?    • Playback control (play/pause/reset)                               ?\n";
    std::cout << "?    • Manual stepping                                                   ?\n";
    std::cout << "?    • Time-based continuous update                                      ?\n";
    std::cout << "?    • Progress tracking                                                 ?\n";
    std::cout << "?    • Edge cases and error handling                                     ?\n";
    std::cout << "??????????????????????????????????????????????????????????????????????????\n";

    try
    {
        test_LoadProgram();
        test_PlaybackControl();
        test_StepExecution();
        test_ContinuousUpdate();
        test_ProgressTracking();
        test_EdgeCases();

        std::cout << "\n";
        std::cout << "??????????????????????????????????????????????????????????????????????????\n";
        std::cout << "?                        ALL TESTS COMPLETED                            ?\n";
        std::cout << "?                                                                        ?\n";
        std::cout << "?  Review the output above for any FAIL results.                         ?\n";
        std::cout << "?  All ? PASS entries indicate working functionality.                    ?\n";
        std::cout << "??????????????????????????????????????????????????????????????????????????\n\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n? EXCEPTION CAUGHT: " << e.what() << "\n\n";
        return 1;
    }

    return 0;
}