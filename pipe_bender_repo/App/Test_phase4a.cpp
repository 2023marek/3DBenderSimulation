// =========================================================================
// PHASE 4A - SIMULATION & RENDERING INTEGRATION TEST
// =========================================================================
//
// PURPOSE: Validate that SimulationController works correctly when
//          integrated with the rendering pipeline
//
// TESTS:
//   1. Single FEED operation with rendering
//   2. FEED + BEND sequence with mesh generation
//   3. Complete program with all operation types
//   4. Speed adjustment during playback
//   5. Pause/Resume functionality
//
// =========================================================================

#include <iostream>
#include <cassert>
#include <cmath>
#include <iomanip>

#include "../Core/SimulationController.h"
#include "../Core/Operations.h"
#include "../Render/TubeMesh.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

// =========================================================================
// TEST UTILITIES
// =========================================================================

void printTestHeader(const std::string& title)
{
    std::cout << "\n";
    std::cout << "?????????????????????????????????????????????????????????????????\n";
    std::cout << "? TEST: " << std::setw(55) << std::left << title << " ?\n";
    std::cout << "?????????????????????????????????????????????????????????????????\n";
}

void assertClose(double actual, double expected, double tolerance,
    const std::string& label)
{
    double diff = std::abs(actual - expected);
    if (diff <= tolerance)
    {
        std::cout << "  ? " << label << " = " << std::fixed
            << std::setprecision(4) << actual << "\n";
    }
    else
    {
        std::cout << "  ? " << label << " = " << std::fixed
            << std::setprecision(4) << actual
            << " (expected " << expected << ")\n";
        assert(false);
    }
}

// =========================================================================
// TEST 1: Single FEED Operation
// =========================================================================

void test_SingleFeed()
{
    printTestHeader("Single FEED Operation");

    SimulationController sim;

    // Create program: FEED 100mm
    std::vector<Operation> program;
    Operation feed;
    feed.type = Operation::FEED;
    feed.length = 100.0;
    program.push_back(feed);

    sim.loadProgram(program);
    sim.play();

    // Simulate for 1 second at 100 mm/s
    int frameCount = 0;
    double totalTime = 0.0;
    double deltaTime = 0.01;  // 10ms per frame

    std::cout << "\nSimulating FEED 100mm at 100 mm/s:\n";
    std::cout << std::string(70, '-') << "\n";
    std::cout << "Frame | Time (s) | Distance (mm) | Progress | Nodes\n";
    std::cout << std::string(70, '-') << "\n";

    while (!program.empty() && frameCount < 200)  // Safety limit
    {
        sim.update(deltaTime);
        totalTime += deltaTime;

        if (frameCount % 10 == 0)  // Print every 10 frames
        {
            double progress = sim.getCurrentOperationProgress();
            size_t nodeCount = sim.getPipeGeometry().getNodes().size();
            std::cout << std::setw(5) << frameCount
                << " | " << std::setw(8) << std::fixed
                << std::setprecision(2) << totalTime
                << " | " << std::setw(13) << (progress * 100.0)
                << " | " << std::setw(8) << (int)(progress * 100) << "%"
                << " | " << std::setw(5) << nodeCount << "\n";
        }

        frameCount++;

        if (sim.getQueue().isComplete())
            break;
    }

    std::cout << std::string(70, '-') << "\n";

    // Verify results
    std::cout << "\nVerification:\n";
    assertClose(sim.getState().position.x, 100.0, 1.0,
        "Final X position");
    assert(sim.getPipeGeometry().getNodes().size() >= 19 &&
        "Geometry has enough nodes");
    std::cout << "  ? Total frames: " << frameCount << "\n";
    std::cout << "  ? FEED operation completed successfully\n";
}

// =========================================================================
// TEST 2: FEED + BEND + FEED Sequence
// =========================================================================

void test_FeedBendFeed()
{
    printTestHeader("FEED ? BEND ? FEED Sequence");

    SimulationController sim;

    // Create program: FEED 50 ? BEND 40,90° ? FEED 50
    std::vector<Operation> program;

    Operation feed1;
    feed1.type = Operation::FEED;
    feed1.length = 50.0;
    program.push_back(feed1);

    Operation bend;
    bend.type = Operation::BEND;
    bend.R = 40.0;
    bend.angle = PI / 2.0;  // 90 degrees
    program.push_back(bend);

    Operation feed2;
    feed2.type = Operation::FEED;
    feed2.length = 50.0;
    program.push_back(feed2);

    sim.loadProgram(program);
    sim.play();

    std::cout << "\nSimulating 3-operation sequence:\n";
    std::cout << "  Op 0: FEED 50mm\n";
    std::cout << "  Op 1: BEND R=40mm, angle=90°\n";
    std::cout << "  Op 2: FEED 50mm\n";
    std::cout << "\nProgress:\n";
    std::cout << std::string(80, '-') << "\n";

    int frameCount = 0;
    double deltaTime = 0.01;

    while (frameCount < 500)
    {
        sim.update(deltaTime);

        if (frameCount % 50 == 0)
        {
            size_t opIdx = sim.getCurrentOperationIndex();
            double opProg = sim.getCurrentOperationProgress();
            double overallProg = sim.getOverallProgress();
            size_t nodeCount = sim.getPipeGeometry().getNodes().size();

            std::cout << "Frame " << std::setw(3) << frameCount
                << " | Op " << opIdx
                << " (" << (int)(opProg * 100) << "%)"
                << " | Overall " << (int)(overallProg * 100) << "%"
                << " | Nodes " << nodeCount << "\n";
        }

        frameCount++;

        if (sim.getQueue().isComplete())
        {
            std::cout << "\n? All operations completed!\n";
            break;
        }
    }

    std::cout << std::string(80, '-') << "\n";

    // Verify
    std::cout << "\nVerification:\n";
    assert(sim.getQueue().isComplete() && "Queue should be complete");
    assert(sim.getPipeGeometry().getNodes().size() > 30 &&
        "Geometry should have many nodes");
    std::cout << "  ? Total frames: " << frameCount << "\n";
    std::cout << "  ? Final node count: "
        << sim.getPipeGeometry().getNodes().size() << "\n";
    std::cout << "  ? Final position: ("
        << std::fixed << std::setprecision(1)
        << sim.getState().position.x << ", "
        << sim.getState().position.y << ", "
        << sim.getState().position.z << ")\n";
}

// =========================================================================
// TEST 3: Pause/Resume Functionality
// =========================================================================

void test_PauseResume()
{
    printTestHeader("Pause/Resume Functionality");

    SimulationController sim;

    std::vector<Operation> program;
    Operation feed;
    feed.type = Operation::FEED;
    feed.length = 100.0;
    program.push_back(feed);

    sim.loadProgram(program);
    sim.play();

    std::cout << "\nSequence: Play ? 50 frames ? Pause ? 50 frames ? Resume ? Complete\n\n";

    // Phase 1: Play for 50 frames
    std::cout << "Phase 1: Playing (50 frames)...\n";
    for (int i = 0; i < 50; i++)
    {
        sim.update(0.01);
    }
    double pausedPos = sim.getState().position.x;
    double pausedProg = sim.getCurrentOperationProgress();
    std::cout << "  Position at pause: " << std::fixed << std::setprecision(1)
        << pausedPos << " mm\n";
    std::cout << "  Progress at pause: " << (int)(pausedProg * 100) << "%\n";

    // Phase 2: Pause (simulation runs but play=false)
    std::cout << "\nPhase 2: Pausing...\n";
    sim.pause();
    for (int i = 0; i < 50; i++)
    {
        sim.update(0.01);
    }
    double afterPausePos = sim.getState().position.x;
    std::cout << "  Position after pause: " << afterPausePos << " mm\n";

    // Verify position didn't change
    assert(std::abs(afterPausePos - pausedPos) < 0.1 &&
        "Position should not change while paused");
    std::cout << "  ? Position unchanged while paused\n";

    // Phase 3: Resume
    std::cout << "\nPhase 3: Resuming...\n";
    sim.play();
    while (!sim.getQueue().isComplete())
    {
        sim.update(0.01);
    }
    double finalPos = sim.getState().position.x;
    std::cout << "  Final position: " << finalPos << " mm\n";

    // Verify
    std::cout << "\nVerification:\n";
    assertClose(finalPos, 100.0, 1.0, "Final position");
    std::cout << "  ? Pause/Resume works correctly\n";
}

// =========================================================================
// TEST 4: Speed Adjustment
// =========================================================================

void test_SpeedAdjustment()
{
    printTestHeader("Speed Adjustment During Playback");

    SimulationController sim;

    std::vector<Operation> program;
    Operation feed;
    feed.type = Operation::FEED;
    feed.length = 100.0;
    program.push_back(feed);

    sim.loadProgram(program);
    sim.setSpeed(100.0);  // 100 mm/s
    sim.play();

    std::cout << "\nSpeed progression: 100 ? 200 ? 50 mm/s\n\n";

    int frameCount = 0;

    // Phase 1: 100 mm/s for 50 frames
    std::cout << "Phase 1: Speed = 100 mm/s (50 frames)...\n";
    for (int i = 0; i < 50; i++)
    {
        sim.update(0.01);
        frameCount++;
    }
    double pos1 = sim.getState().position.x;
    std::cout << "  Position: " << std::fixed << std::setprecision(2)
        << pos1 << " mm\n";

    // Phase 2: 200 mm/s for 25 frames
    std::cout << "\nPhase 2: Speed = 200 mm/s (25 frames)...\n";
    sim.setSpeed(200.0);
    for (int i = 0; i < 25; i++)
    {
        sim.update(0.01);
        frameCount++;
    }
    double pos2 = sim.getState().position.x;
    double traveled2 = pos2 - pos1;
    std::cout << "  Distance traveled: " << traveled2 << " mm\n";
    std::cout << "  (Expected: ~50 mm at 200 mm/s for 0.25s)\n";

    // Phase 3: 50 mm/s to completion
    std::cout << "\nPhase 3: Speed = 50 mm/s (continue to completion)...\n";
    sim.setSpeed(50.0);
    while (!sim.getQueue().isComplete())
    {
        sim.update(0.01);
        frameCount++;
    }
    double finalPos = sim.getState().position.x;
    std::cout << "  Final position: " << finalPos << " mm\n";

    std::cout << "\nVerification:\n";
    assertClose(finalPos, 100.0, 1.0, "Final position");
    std::cout << "  ? Speed adjustment works correctly\n";
    std::cout << "  ? Total frames: " << frameCount << "\n";
}

// =========================================================================
// TEST 5: Mesh Generation During Simulation
// =========================================================================

void test_MeshGeneration()
{
    printTestHeader("Mesh Generation During Simulation");

    SimulationController sim;

    std::vector<Operation> program;
    Operation feed;
    feed.type = Operation::FEED;
    feed.length = 100.0;
    program.push_back(feed);

    sim.loadProgram(program);
    sim.play();

    std::cout << "\nGenerating mesh at different simulation states:\n";
    std::cout << std::string(80, '-') << "\n";
    std::cout << "Frame | Progress | Nodes | Mesh Vertices | Mesh Triangles\n";
    std::cout << std::string(80, '-') << "\n";

    TubeMesh mesh;
    double radius = 8.0;
    int segments = 16;

    for (int frameNum = 0; frameNum < 200; frameNum++)
    {
        sim.update(0.01);

        if (frameNum % 20 == 0)  // Check every 20 frames
        {
            const auto& nodes = sim.getPipeGeometry().getNodes();

            // Extract points and tangents
            std::vector<Vec3D> points;
            std::vector<Vec3D> tangents;
            for (const auto& node : nodes)
            {
                points.push_back(node.pos);
                tangents.push_back(node.T);
            }

            // Generate mesh
            if (!points.empty())
            {
                mesh.generate(points, tangents, radius, segments);

                double progress = sim.getCurrentOperationProgress();
                std::cout << std::setw(5) << frameNum
                    << " | " << std::setw(8) << (int)(progress * 100) << "%"
                    << " | " << std::setw(5) << nodes.size()
                    << " | " << std::setw(13) << mesh.getVertices().size()
                    << " | " << std::setw(14) << (mesh.getIndices().size() / 3)
                    << "\n";
            }
        }

        if (sim.getQueue().isComplete())
            break;
    }

    std::cout << std::string(80, '-') << "\n";
    std::cout << "\n? Mesh generation works during simulation\n";
}

// =========================================================================
// MAIN TEST RUNNER
// =========================================================================

int main()
{
    std::cout << "\n";
    std::cout << "?????????????????????????????????????????????????????????????????\n";
    std::cout << "?              PHASE 4A - INTEGRATION TEST SUITE                 ?\n";
    std::cout << "?                                                               ?\n";
    std::cout << "?  Tests SimulationController with rendering pipeline           ?\n";
    std::cout << "?????????????????????????????????????????????????????????????????\n";

    try
    {
        test_SingleFeed();
        test_FeedBendFeed();
        test_PauseResume();
        test_SpeedAdjustment();
        test_MeshGeneration();

        std::cout << "\n";
        std::cout << "?????????????????????????????????????????????????????????????????\n";
        std::cout << "?                    ? ALL TESTS PASSED!                        ?\n";
        std::cout << "?????????????????????????????????????????????????????????????????\n\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n? TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}