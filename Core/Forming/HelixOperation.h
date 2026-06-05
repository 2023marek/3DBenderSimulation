#pragma once

// =====================================================
// HELIX OPERATION
//
// High-level forming command for helix / coil-like shapes.
//
// Current scope:
// - geometric helix
// - machine kinematics helper values
//
// Not included yet:
// - springback
// - material strain
// - tension / stress
// =====================================================

struct HelixOperation
{
    enum class InputMode
    {
        RadiusPitch,
        CurvatureTorsion
    };

    InputMode inputMode =
        InputMode::RadiusPitch;

    // =====================================================
    // COMMON
    // =====================================================

    // Pipe arc length to form into helix.
    double length = 0.0;

    // =====================================================
    // RADIUS / PITCH INPUT
    //
    // Circular helix:
    //
    // x = r cos(u)
    // y = r sin(u)
    // z = b u
    //
    // where:
    // b = pitch / (2*pi)
    //
    // curvature:
    // kappa = r / (r*r + b*b)
    //
    // torsion:
    // tau = b / (r*r + b*b)
    // =====================================================

    double helixRadius = 0.0;
    double pitch = 0.0;

    // =====================================================
    // DIRECT CURVATURE / TORSION INPUT
    //
    // Useful for tests, advanced forming, or future solver output.
    // =====================================================

    double curvature = 0.0;
    double torsion = 0.0;

    // =====================================================
    // MACHINE KINEMATICS
    //
    // feedSpeed:
    //      ds / dt
    //
    // angularSpeed:
    //      approximate rotation rate for helix machine
    //
    // For simple torsion-driven control:
    //      omega = tau * feedSpeed
    // =====================================================

    double feedSpeed = 0.0;
    double angularSpeed = 0.0;
};