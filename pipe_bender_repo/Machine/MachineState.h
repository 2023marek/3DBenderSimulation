#pragma once
#include "../Core/Math/Vec3D.h"
#include <iostream>

/// Represents the current state of the pipe bending machine
/// Tracks position, orientation, and time
class MachineState
{
public:
    // ===== GEOMETRY =====
    Vec3D position{ 0.0, 0.0, 0.0 };      // Current pipe position (mm)
    Vec3D tangent{ 1.0, 0.0, 0.0 };       // Current pipe direction

    // ===== OPERATION PROGRESS =====
    double progressMm = 0.0;               // mm done in current operation
    double totalDistanceMm = 0.0;          // total mm processed (all operations)

    // ===== TIME =====
    double currentTime = 0.0;              // elapsed simulation time (seconds)

    // ===== STATUS =====
    enum class Status
    {
        IDLE,                              // Waiting for program
        RUNNING,                           // Actively processing
        PAUSED,                            // Paused mid-operation
        COMPLETED,                         // Program finished
        ERROR                              // Error state
    };

    Status status = Status::IDLE;
    std::string lastError;
    std::string lastWarning;

    // ===== METHODS =====

    /// Reset to initial state
    void reset()
    {
        position = { 0.0, 0.0, 0.0 };
        tangent = { 1.0, 0.0, 0.0 };
        progressMm = 0.0;
        totalDistanceMm = 0.0;
        currentTime = 0.0;
        status = Status::IDLE;
        lastError.clear();
        lastWarning.clear();
    }

    /// Update position (for FEED operation)
    /// @param distance mm to move forward
    void feedForward(double distance)
    {
        Vec3D movement = tangent * distance;
        position = position + movement;
        progressMm += distance;
        totalDistanceMm += distance;
    }

    /// Update angle (for BEND operation)
    /// @param angleDelta radians to bend
    void bendAngle(double angleDelta)
    {
        progressMm += angleDelta; // Track as "progress units"
        // Note: PipeAxis3D handles actual geometry
    }

    /// Set status
    void setStatus(Status s) { status = s; }

    /// Get status string
    std::string getStatusString() const
    {
        switch (status)
        {
        case Status::IDLE:      return "IDLE";
        case Status::RUNNING:   return "RUNNING";
        case Status::PAUSED:    return "PAUSED";
        case Status::COMPLETED: return "COMPLETED";
        case Status::ERROR:     return "ERROR";
        default:                return "UNKNOWN";
        }
    }

    /// Debug output
    void print() const
    {
        std::cout << "\n=== MACHINE STATE ===\n";
        std::cout << "Status: " << getStatusString() << "\n";
        std::cout << "Position: (" << position.x << ", " << position.y << ", " << position.z << ")\n";
        std::cout << "Tangent: (" << tangent.x << ", " << tangent.y << ", " << tangent.z << ")\n";
        std::cout << "Progress: " << progressMm << " mm (current op)\n";
        std::cout << "Total: " << totalDistanceMm << " mm (all ops)\n";
        std::cout << "Time: " << currentTime << " s\n";

        if (!lastWarning.empty())
            std::cout << " Warning: " << lastWarning << "\n";
        if (!lastError.empty())
            std::cout << " Error: " << lastError << "\n";
    }
};