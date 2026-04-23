#pragma once
#include <vector>
#include "Operations.h"
#include <iostream>

/// Manages the queue of operations to execute
/// Provides:
///   - Current operation access
///   - Progress tracking
///   - State management (running, complete)
class OperationQueue
{
public:
    /// Load operations from vector
    void load(const std::vector<Operation>& ops)
    {
        operations = ops;
        currentIndex = 0;
        std::cout << " Queue loaded: " << operations.size() << " operations\n";
    }

    /// Get current operation (nullptr if queue complete)
    const Operation* getCurrent() const
    {
        if (currentIndex < operations.size())
            return &operations[currentIndex];
        return nullptr;
    }

    /// Advance to next operation
    void nextOperation()
    {
        if (currentIndex < operations.size())
        {
            std::cout << "  ? Operation " << currentIndex << " complete\n";
            currentIndex++;
        }
    }

    /// Check if all operations complete
    bool isComplete() const
    {
        return currentIndex >= operations.size();
    }

    /// Get progress (0.0 to 1.0)
    double getProgress() const
    {
        if (operations.empty()) return 1.0;
        return static_cast<double>(currentIndex) / operations.size();
    }

    /// Reset to beginning
    void reset()
    {
        currentIndex = 0;
        std::cout << "?? Queue reset\n";
    }

    // Getters
    size_t getTotalOperations() const { return operations.size(); }
    size_t getCurrentIndex() const { return currentIndex; }

    /// Debug output
    void print() const
    {
        std::cout << "\n=== OPERATION QUEUE ===\n";
        std::cout << "Progress: " << currentIndex << "/" << operations.size() << "\n";
        for (size_t i = 0; i < operations.size(); ++i)
        {
            if (i == currentIndex)
                std::cout << "? ";  // Current operation
            else
                std::cout << "  ";

            std::cout << i << ": ";
            operations[i].print();
        }
        std::cout << "\n";
    }

private:
    std::vector<Operation> operations;
    size_t currentIndex = 0;
};
