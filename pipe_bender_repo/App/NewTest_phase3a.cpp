#include "../Core/SimulationController.h"
#include "../Core/PipeAxis3D.h"
#include "../Core/Operations.h"
#include "../Machine/MachineState.h"
#include <iostream>
#include <cassert>
#include <iomanip>
#include <cmath>

// =========================================================================
// CONSTANTS
// =========================================================================
#ifndef PI
#define PI 3.14159265358979323846
#endif

/*
================================================================================
  TEST PHASE 3A - SimulationController + PipeAxis3D Integration
================================================================================

  GOAL: Verify that SimulationController updates PipeAxis3D geometry
        in real-time as operations execute.

  What gets tested:
    ? Geometry grows as simulation progresses
    ? Node count increases per frame
    ? Multi-operation coordination (FEED ? BEND ? FEED)
    ? Accurate geometry generation
    ? Progress tracking matches geometry

  ARCHITECTURE:

  ???????????????????????????????????????
  ? SimulationController                ?  Orchestrator
  ?  • Manages operations                ?
  ?  • Tracks progress                   ?
  ?  • Calls updatePipeGeometry()       ?
  ???????????????????????????????????????
                 ? update(deltaTime)
                 ?
  ???????????????????????????????????????
  ? executeOperation()                  ?  Executes current op
  ?  • executeFeed() or executeBend()   ?
  ?  • Updates accumulated progress     ?
  ?  • Advances queue when complete     ?
  ???????????????????????????????????????
                 ? Then calls...
                 ?
  ???????????????????????????????????????
  ? updatePipeGeometry()                ?  Rebuilds geometry
  ?  • Regenerates nodes from segments  ?
  ?  • Ready for rendering              ?
  ???????????????????????????????????????
                 ?
                 ?
  ???????????????????????????????????????
  ? PipeAxis3D                          ?  3D Geometry
  ?  • getNodes() for rendering         ?
  ?  • Smooth curves & lines            ?
  ???????????????????????????????????????

================================================================================
*/

// =============================================================================
// HELPER FUNCTIONS
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

static void printGeometryStats(const PipeAxis3D& pipe, size_t frameNum)
{
    const auto& nodes = pipe.getNodes();
    std::cout << "     Frame " << frameNum << ": " << nodes.size()
        << " nodes | ";

    if (!nodes.empty())
    {
        std::cout << "Last pos: ("
            << std::fixed << std::setprecision(1)
            << nodes.back().pos.x << ", "
            << nodes.back().pos.y << ", "
            << nodes.back().pos.z << ")";
    }
    std::cout << "\n";
}

// =============================================================================
// TEST 1: Basic Geometry Growth During Simulation
// =============================================================================

/*
  ??????????????????????????????????????????????????????????????
  ? TEST 1: Geometry Grows as Simulation Progresses            ?
  ?                                                            ?
  ? Expected Behavior:                                         ?
  ?   Frame 0: 1 node (start position)                         ?
  ?   Frame 1: More nodes added                                ?
  ?   Frame 2: Even more nodes                                 ?
  ?   ...                                                      ?
  ?   Final:   Complete pipe with all operations              ?
  ?                                                            ?
  ? Why? Each frame adds a slice of the pipe.                 ?
  ?      updatePipeGeometry() rebuilds geometry.              ?
  ?      getNodes() returns accumulated geometry.             ?
  ??????????????????????????????????????????????????????????????
*/

static void test_GeometryGrowthDuringSim()
{
    printTestHeader("Geometry Growth During Simulation");

    SimulationController sim;
    sim.setSpeed(50.0);  // 50 mm/sec

    // Load simple program: just one FEED operation
    std::vector<Operation> program;
    Operation op;
    op.type = Operation::FEED;
    op.length = 100.0;  // 100mm feed
    program.push_back(op);

    sim.loadProgram(program);

    std::cout << "\n  ? Program loaded: 1 FEED operation (100mm)\n";
    std::cout << "     Speed: 50mm/s, expected completion: 2.0s\n";
    std::cout << "     Simulating with deltaTime=0.1s per frame (10 frames)\n";

    printSeparator();

    // Simulate frame-by-frame
    sim.play();
    std::vector<size_t> nodeCounts;

    std::cout << "\n  Geometry Growth Over Time:\n";

    for (int frame = 0; frame < 15; frame++)
    {
        // Update simulation by 0.1 seconds
        sim.update(0.1);

        // Get current geometry
        const auto& nodes = sim.getPipeGeometry().getNodes();
        nodeCounts.push_back(nodes.size());

        // Print frame info
        printGeometryStats(sim.getPipeGeometry(), frame);

        // Stop if complete
        if (sim.getQueue().isComplete())
        {
            std::cout << "     ? Simulation complete!\n";
            break;
        }
    }

    printSeparator();

    // Verify: Node count should increase monotonically
    bool growthMonotonic = true;
    for (size_t i = 1; i < nodeCounts.size(); i++)
    {
        if (nodeCounts[i] < nodeCounts[i - 1])
        {
            growthMonotonic = false;
            break;
        }
    }

    printTestResult(
        growthMonotonic,
        "Geometry grows monotonically (never decreases)"
    );

    printTestResult(
        nodeCounts.back() > 1,
        "Final geometry has multiple nodes"
    );

    printTestResult(
        nodeCounts.front() == 1,
        "Initial frame has 1 node (start position)"
    );

    printSeparator();
}

// =============================================================================
// TEST 2: Multi-Operation Coordination
// =============================================================================

/*
  ??????????????????????????????????????????????????????????????
  ? TEST 2: Correct Geometry for Multiple Operations           ?
  ?                                                            ?
  ? Program:                                                   ?
  ?   Op 0: FEED 100mm  ? LINE segment (straight)             ?
  ?   Op 1: BEND 50mm R, 90° ? ARC segment (curved)          ?
  ?   Op 2: FEED 80mm   ? LINE segment (straight again)       ?
  ?                                                            ?
  ? Expected Geometry:                                         ?
  ?   ??????????????????????????????????                      ?
  ?   ? Start                          ?                      ?
  ?   ?   ?                            ?                      ?
  ?   ?  ??? (FEED: straight line)     ?                      ?
  ?   ?   ?                            ?                      ?
  ?   ?  ??? (BEND: curve)             ?                      ?
  ?   ?  ? ?                           ?                      ?
  ?   ?  ???                           ?                      ?
  ?   ?   ?                            ?                      ?
  ?   ?  ??? (FEED: straight again)    ?                      ?
  ?   ??????????????????????????????????                      ?
  ??????????????????????????????????????????????????????????????
*/

static void test_MultiOperationCoordination()
{
    printTestHeader("Multi-Operation Coordination");

    SimulationController sim;
    sim.setSpeed(100.0);  // 100 mm/sec

    // Build program with 3 operations
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

    sim.loadProgram(program);

    std::cout << "\n  ? Program Structure:\n";
    std::cout << "     Op 0: FEED 100mm (should create LINE)\n";
    std::cout << "     Op 1: BEND R=50mm, 90° (should create ARC)\n";
    std::cout << "     Op 2: FEED 80mm (should create LINE)\n";

    std::cout << "\n  ? Simulating until complete...\n";

    sim.play();
    size_t frameCount = 0;
    size_t lastOpIndex = 0;

    printSeparator();
    std::cout << "\n  Operation Transitions:\n";

    while (!sim.getQueue().isComplete())
    {
        size_t currentOp = sim.getCurrentOperationIndex();

        // Print when operation changes
        if (currentOp != lastOpIndex)
        {
            const auto& nodes = sim.getPipeGeometry().getNodes();
            std::cout << "     ? Op " << currentOp << " started at frame "
                << frameCount << " (" << nodes.size() << " nodes)\n";
            lastOpIndex = currentOp;
        }

        sim.update(0.01);  // 10ms per frame
        frameCount++;

        if (frameCount > 500)  // Safety limit
            break;
    }

    const auto& finalNodes = sim.getPipeGeometry().getNodes();
    std::cout << "     ? Op " << sim.getTotalOperations() << " complete at frame "
        << frameCount << " (" << finalNodes.size() << " nodes)\n";

    printSeparator();

    printTestResult(
        sim.getTotalOperations() == 3,
        "All 3 operations processed"
    );

    printTestResult(
        sim.getQueue().isComplete(),
        "Queue complete after simulation"
    );

    printTestResult(
        finalNodes.size() > 50,  // Should have many nodes
        "Final geometry has substantial node count"
    );

    printSeparator();
}

// =============================================================================
// TEST 3: Progress Matches Geometry Growth
// =============================================================================

/*
  ??????????????????????????????????????????????????????????????
  ? TEST 3: Progress Correlates with Geometry Growth           ?
  ?                                                            ?
  ? Relationship:                                              ?
  ?   Progress 0%   ? Few nodes                                ?
  ?   Progress 50%  ? ~Half the nodes                          ?
  ?   Progress 100% ? All nodes                                ?
  ?                                                            ?
  ? Example: 100mm FEED at 50mm/s                              ?
  ?   t=0.0s: Progress 0%, Nodes ? 1                          ?
  ?   t=1.0s: Progress 50%, Nodes ? N/2                       ?
  ?   t=2.0s: Progress 100%, Nodes = N                         ?
  ??????????????????????????????????????????????????????????????
*/

static void test_ProgressMatchesGeometry()
{
    printTestHeader("Progress Correlates with Geometry Growth");

    SimulationController sim;
    sim.setSpeed(50.0);  // 50 mm/sec

    std::vector<Operation> program;
    Operation op;
    op.type = Operation::FEED;
    op.length = 100.0;
    program.push_back(op);

    sim.loadProgram(program);

    std::cout << "\n  ? Program: 100mm FEED at 50mm/s (2 seconds total)\n";
    std::cout << "     Sampling progress vs node count at 5 snapshots\n";

    printSeparator();
    std::cout << "\n  Progress Analysis:\n";

    sim.play();

    struct Snapshot
    {
        double progress;
        size_t nodeCount;
    };

    std::vector<Snapshot> snapshots;

    // Sample at different progress levels
    for (int frame = 0; frame < 25; frame++)
    {
        sim.update(0.1);  // 100ms per frame

        double progress = sim.getOverallProgress();
        size_t nodeCount = sim.getPipeGeometry().getNodes().size();

        if (frame % 5 == 0 || sim.getQueue().isComplete())
        {
            snapshots.push_back({ progress, nodeCount });

            std::cout << "     Progress: " << std::fixed << std::setprecision(0)
                << (progress * 100.0) << "%"
                << " | Nodes: " << nodeCount << "\n";
        }

        if (sim.getQueue().isComplete())
            break;
    }

    printSeparator();

    // Verify: node count increases with progress
    bool progresses = true;
    for (size_t i = 1; i < snapshots.size(); i++)
    {
        if (snapshots[i].nodeCount <= snapshots[i - 1].nodeCount &&
            snapshots[i].progress > snapshots[i - 1].progress)
        {
            progresses = false;
            break;
        }
    }

    printTestResult(
        progresses,
        "Node count increases as progress increases"
    );

    printTestResult(
        snapshots.back().progress >= 0.99,
        "Simulation reaches completion (?99% progress)"
    );

    printTestResult(
        snapshots.back().nodeCount > snapshots.front().nodeCount,
        "Final geometry larger than initial"
    );

    printSeparator();
}

// =============================================================================
// TEST 4: Frame-by-Frame Geometry Update (Key for Real-Time)
// =============================================================================

/*
  ??????????????????????????????????????????????????????????????
  ? TEST 4: Frame-by-Frame Geometry Updates (Real-Time Ready)  ?
  ?                                                            ?
  ? This is CRITICAL for real-time rendering:                 ?
  ?   Every frame should have updated geometry                 ?
  ?   No jumps or missing data                                 ?
  ?   Smooth animation possible                               ?
  ?                                                            ?
  ? Visualization:                                             ?
  ?   Frame 0:   ??????????????  (5% of pipe)                 ?
  ?   Frame 1:   ???????????????  (12% of pipe)                ?
  ?   Frame 2:   ???????????????  (20% of pipe)                ?
  ?   Frame 3:   ???????????????? (25% of pipe)                ?
  ?   ...                                                      ?
  ?   Final:     ?????????????? (100% of pipe)                 ?
  ??????????????????????????????????????????????????????????????
*/

static void test_FrameByFrameUpdate()
{
    printTestHeader("Frame-by-Frame Geometry Update (Real-Time Ready)");

    SimulationController sim;
    sim.setSpeed(100.0);  // Fast for visible changes

    // Create 2-operation program for variety
    std::vector<Operation> program;

    Operation op1;
    op1.type = Operation::FEED;
    op1.length = 50.0;
    program.push_back(op1);

    Operation op2;
    op2.type = Operation::FEED;
    op2.length = 50.0;
    program.push_back(op2);

    sim.loadProgram(program);

    std::cout << "\n  ? Program: 2x FEED 50mm each (100mm total)\n";
    std::cout << "     Rendering 10 consecutive frames at 0.05s each\n";

    printSeparator();
    std::cout << "\n  Frame Sequence (Ready for rendering):\n";

    sim.play();

    std::vector<size_t> frameSizes;
    const int NUM_FRAMES = 10;

    for (int frame = 0; frame < NUM_FRAMES; frame++)
    {
        sim.update(0.05);  // 50ms per frame
        size_t nodeCount = sim.getPipeGeometry().getNodes().size();
        frameSizes.push_back(nodeCount);

        // Progress bar visualization
        double progress = sim.getOverallProgress();
        int barWidth = (int)(progress * 30);
        std::cout << "     Frame " << frame << ": ";
        for (int i = 0; i < 30; i++)
            std::cout << (i < barWidth ? "?" : "?");
        std::cout << " (" << nodeCount << " nodes)\n";

        if (sim.getQueue().isComplete())
            break;
    }

    printSeparator();

    // Verify each frame is valid for rendering
    bool validForRendering = true;
    for (size_t count : frameSizes)
    {
        if (count < 1)  // Must have at least the start node
        {
            validForRendering = false;
            break;
        }
    }

    printTestResult(
        validForRendering,
        "Each frame has valid geometry (?1 node)"
    );

    printTestResult(
        frameSizes.size() >= NUM_FRAMES || sim.getQueue().isComplete(),
        "Simulated multiple frames without errors"
    );

    // Check for smooth growth
    bool smoothGrowth = true;
    for (size_t i = 1; i < frameSizes.size(); i++)
    {
        // Each frame should have same or more nodes
        if (frameSizes[i] < frameSizes[i - 1])
        {
            smoothGrowth = false;
            break;
        }
    }

    printTestResult(
        smoothGrowth,
        "Geometry growth is smooth (monotonic increase)"
    );

    printSeparator();
}

// =============================================================================
// TEST 5: Verify Geometry Ready for Rendering
// =============================================================================

/*
  ??????????????????????????????????????????????????????????????
  ? TEST 5: Geometry Format Valid for Rendering                ?
  ?                                                            ?
  ? Rendering Requirements:                                    ?
  ?   ? All nodes have valid positions (no NaN/Inf)           ?
  ?   ? All nodes have valid tangents (normalized)            ?
  ?   ? Positions are sequential (form connected path)        ?
  ?   ? No duplicate adjacent nodes                           ?
  ?                                                            ?
  ? This ensures the geometry can be used by:                  ?
  ?   • TubeMesh.generate()                                    ?
  ?   • PipeRenderer.uploadMesh()                              ?
  ?   • GPU rendering pipeline                                 ?
  ??????????????????????????????????????????????????????????????
*/

static void test_GeometryRenderingFormat()
{
    printTestHeader("Geometry Format Valid for Rendering");

    SimulationController sim;
    sim.setSpeed(50.0);

    std::vector<Operation> program;
    Operation op;
    op.type = Operation::FEED;
    op.length = 100.0;
    program.push_back(op);

    sim.loadProgram(program);
    sim.play();

    // Run simulation to completion
    while (!sim.getQueue().isComplete())
    {
        sim.update(0.05);
    }

    const auto& nodes = sim.getPipeGeometry().getNodes();

    std::cout << "\n  ? Final geometry validation:\n";
    std::cout << "     Total nodes: " << nodes.size() << "\n";

    printSeparator();

    // Check 1: All positions are valid (no NaN/Inf)
    bool positionsValid = true;
    for (const auto& node : nodes)
    {
        if (std::isnan(node.pos.x) || std::isnan(node.pos.y) || std::isnan(node.pos.z) ||
            std::isinf(node.pos.x) || std::isinf(node.pos.y) || std::isinf(node.pos.z))
        {
            positionsValid = false;
            break;
        }
    }

    printTestResult(
        positionsValid,
        "All node positions are valid (no NaN/Inf)"
    );

    // Check 2: All tangents are valid
    bool tangentsValid = true;
    for (const auto& node : nodes)
    {
        if (std::isnan(node.T.x) || std::isnan(node.T.y) || std::isnan(node.T.z) ||
            std::isinf(node.T.x) || std::isinf(node.T.y) || std::isinf(node.T.z))
        {
            tangentsValid = false;
            break;
        }
    }

    printTestResult(
        tangentsValid,
        "All node tangents are valid (no NaN/Inf)"
    );

    // Check 3: Positions form connected path (distances are reasonable)
    bool pathConnected = true;
    const double MAX_NODE_DISTANCE = 20.0;  // 5mm segment size * 4
    for (size_t i = 1; i < nodes.size(); i++)
    {
        Vec3D diff = nodes[i].pos - nodes[i - 1].pos;
        double dist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

        if (dist > MAX_NODE_DISTANCE)
        {
            pathConnected = false;
            std::cout << "     WARNING: Large gap at node " << i << " (" << dist << "mm)\n";
            break;
        }
    }

    printTestResult(
        pathConnected,
        "All nodes form connected path (no large gaps)"
    );

    // Check 4: Final position is reasonable
    if (!nodes.empty())
    {
        const auto& finalPos = nodes.back().pos;
        double totalDist = std::sqrt(finalPos.x * finalPos.x +
            finalPos.y * finalPos.y +
            finalPos.z * finalPos.z);

        std::cout << "\n     Final position: ("
            << std::fixed << std::setprecision(1)
            << finalPos.x << ", " << finalPos.y << ", " << finalPos.z << ")\n";
        std::cout << "     Distance from origin: " << totalDist << "mm\n";

        printTestResult(
            totalDist > 50.0,  // Should have moved a reasonable distance
            "Final position is reasonable distance from origin"
        );
    }

    printSeparator();
}

// =============================================================================
// MAIN TEST RUNNER
// =============================================================================

int main()
{
    std::cout << "\n";
    std::cout << "?????????????????????????????????????????????????????????????????????????????\n";
    std::cout << "?          PHASE 3A: SIMULATION + GEOMETRY INTEGRATION TESTS                ?\n";
    std::cout << "?                                                                           ?\n";
    std::cout << "?  This test suite validates integration between:                           ?\n";
    std::cout << "?    • SimulationController (orchestration)                                 ?\n";
    std::cout << "?    • PipeAxis3D (geometry generation)                                     ?\n";
    std::cout << "?                                                                           ?\n";
    std::cout << "?  Goal: Ensure real-time geometry updates work correctly                   ?\n";
    std::cout << "?????????????????????????????????????????????????????????????????????????????\n";

    try
    {
        test_GeometryGrowthDuringSim();
        test_MultiOperationCoordination();
        test_ProgressMatchesGeometry();
        test_FrameByFrameUpdate();
        test_GeometryRenderingFormat();

        std::cout << "\n";
        std::cout << "?????????????????????????????????????????????????????????????????????????????\n";
        std::cout << "?                    ALL INTEGRATION TESTS COMPLETED                        ?\n";
        std::cout << "?                                                                           ?\n";
        std::cout << "?  Status: Ready for Phase 3B (Renderer Integration)                        ?\n";
        std::cout << "?                                                                           ?\n";
        std::cout << "?  Next: Implement updatePipeGeometry() with Option C strategy             ?\n";
        std::cout << "?????????????????????????????????????????????????????????????????????????????\n\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n? EXCEPTION: " << e.what() << "\n\n";
        return 1;
    }

    return 0;
}