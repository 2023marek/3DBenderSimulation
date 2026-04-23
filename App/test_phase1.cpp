#include <iostream>
#include <cassert>
#include "../Core/Operations.h"
#include "../Core/OperationQueue.h"
#include "../Machine/MachineState.h"
#include "../Core/ProgramLoader.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

// ===== TEST 1: Operation.print() =====
void test_operation_print()
{
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "TEST 1: Operation::print()\n";
    std::cout << std::string(50, '=') << "\n";

    // Test FEED
    Operation feed;
    feed.type = Operation::FEED;
    feed.length = 100.0;
    std::cout << "? FEED operation: ";
    feed.print();

    // Test BEND
    Operation bend;
    bend.type = Operation::BEND;
    bend.R = 50.0;
    bend.angle = PI / 3;  // 60 degrees
    bend.dir = BendDirection::CCW;
    std::cout << "? BEND operation: ";
    bend.print();

    // Test with progress
    bend.progress = 0.5;
    std::cout << "? BEND with progress: ";
    bend.print();

    std::cout << "? TEST 1 PASSED\n";
}

// ===== TEST 2: OperationQueue =====
void test_operation_queue()
{
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "TEST 2: OperationQueue\n";
    std::cout << std::string(50, '=') << "\n";

    std::vector<Operation> ops;

    // Create test operations
    Operation feed;
    feed.type = Operation::FEED;
    feed.length = 100.0;
    ops.push_back(feed);

    Operation bend;
    bend.type = Operation::BEND;
    bend.R = 50.0;
    bend.angle = PI / 3;
    ops.push_back(bend);

    // Load into queue
    OperationQueue queue;
    queue.load(ops);

    // Test initial state
    assert(queue.getTotalOperations() == 2);
    assert(queue.getCurrentIndex() == 0);
    assert(!queue.isComplete());
    assert(queue.getProgress() == 0.0);

    std::cout << "? Queue initialized correctly\n";
    queue.print();

    // Test next operation
    queue.nextOperation();
    assert(queue.getCurrentIndex() == 1);
    assert(queue.getProgress() == 0.5);
    std::cout << "? Queue advanced to next operation\n";

    // Test completion
    queue.nextOperation();
    assert(queue.isComplete());
    std::cout << "? Queue marked as complete\n";

    // Test reset
    queue.reset();
    assert(queue.getCurrentIndex() == 0);
    std::cout << "? Queue reset successfully\n";

    std::cout << "? TEST 2 PASSED\n";
}

// ===== TEST 3: MachineState =====
void test_machine_state()
{
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "TEST 3: MachineState\n";
    std::cout << std::string(50, '=') << "\n";

    MachineState state;

    // Test initial state
    assert(state.position.x == 0.0);
    assert(state.position.y == 0.0);
    assert(state.position.z == 0.0);
    assert(state.status == MachineState::Status::IDLE);
    std::cout << "? Initial state correct\n";

    state.print();

    // Test feeding forward
    state.feedForward(50.0);
    assert(state.totalDistanceMm == 50.0);
    assert(state.position.x == 50.0);  // tangent is (1,0,0)
    std::cout << "? Feed forward 50mm successful\n";

    // Test status change
    state.setStatus(MachineState::Status::RUNNING);
    assert(state.status == MachineState::Status::RUNNING);
    std::cout << "? Status change successful\n";

    // Test reset
    state.reset();
    assert(state.position.x == 0.0);
    assert(state.status == MachineState::Status::IDLE);
    std::cout << "? Reset successful\n";

    std::cout << "? TEST 3 PASSED\n";
}

// ===== TEST 4: ProgramLoader =====
void test_program_loader()
{
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "TEST 4: ProgramLoader\n";
    std::cout << std::string(50, '=') << "\n";

    auto result = ProgramLoader::loadProgram("tube_program.txt");

    if (!result.success)
    {
        std::cerr << "??  Cannot load tube_program.txt (file may not exist)\n";
        std::cerr << "This test requires the program file to be present\n";
        return;
    }

    assert(result.operations.size() > 0);
    std::cout << "? Loaded " << result.operations.size() << " operations\n";

    // Print loaded operations
    for (size_t i = 0; i < result.operations.size(); ++i)
    {
        std::cout << "  " << i << ": ";
        result.operations[i].print();
    }

    std::cout << "? TEST 4 PASSED\n";
}

// ===== MAIN TEST RUNNER =====
int main()
{
    std::cout << "\n";
    std::cout << "????????????????????????????????????????????????????????\n";
    std::cout << "?         PHASE 1: UNIT TESTS                         ?\n";
    std::cout << "?    ProgramLoader, OperationQueue, MachineState      ?\n";
    std::cout << "????????????????????????????????????????????????????????\n";

    try
    {
        test_operation_print();
        test_operation_queue();
        test_machine_state();
        test_program_loader();

        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "?? ALL TESTS PASSED!\n";
        std::cout << std::string(50, '=') << "\n\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n? TEST FAILED: " << e.what() << "\n";
        return -1;
    }
}