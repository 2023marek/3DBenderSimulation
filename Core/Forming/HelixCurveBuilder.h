#pragma once

#include <cmath>
#include <iostream>

#include "Core/Curve/PipeCurveSegment.h"
#include "Core/Forming/HelixOperation.h"

// =====================================================
// HELIX CURVE BUILDER
//
// Converts HelixOperation into PipeCurveSegment.
//
// Current scope:
// - geometric helix
// - curvature / torsion calculation
// - simple machine kinematic helper
//
// Not included yet:
// - springback
// - material model
// - tension / strain / stress
// =====================================================

class HelixCurveBuilder
{
public:
    struct Result
    {
        PipeCurveSegment segment;

        double curvature = 0.0;
        double torsion = 0.0;

        double helixRadius = 0.0;
        double pitch = 0.0;
        double b = 0.0;

        double arcLengthPerTurn = 0.0;
        double numberOfTurns = 0.0;

        double angularSpeed = 0.0;

        bool valid = false;
    };

    static Result build(
        const HelixOperation& op)
    {
        Result result;

        result.helixRadius =
            op.helixRadius;

        result.pitch =
            op.pitch;

        if (op.length <= 0.0)
        {
            std::cerr << "[HELIX BUILDER ERROR] length <= 0\n";
            return result;
        }

        double kappa = 0.0;
        double tau = 0.0;

        if (op.inputMode == HelixOperation::InputMode::RadiusPitch)
        {
            if (op.helixRadius <= 1e-9)
            {
                std::cerr << "[HELIX BUILDER ERROR] helixRadius <= 0\n";
                return result;
            }

            if (std::abs(op.pitch) <= 1e-9)
            {
                std::cerr << "[HELIX BUILDER ERROR] pitch too small\n";
                return result;
            }

            constexpr double pi =
                3.14159265358979323846;

            double r =
                op.helixRadius;

            double b =
                op.pitch / (2.0 * pi);

            double denom =
                r * r + b * b;

            if (denom <= 1e-12)
                return result;

            kappa =
                r / denom;

            tau =
                b / denom;

            result.b =
                b;

            result.arcLengthPerTurn =
                2.0 * pi * std::sqrt(denom);

            result.numberOfTurns =
                op.length / result.arcLengthPerTurn;
        }
        else if (op.inputMode == HelixOperation::InputMode::CurvatureTorsion)
        {
            kappa =
                op.curvature;

            tau =
                op.torsion;

            if (std::abs(kappa) < 1e-12
                && std::abs(tau) < 1e-12)
            {
                std::cerr << "[HELIX BUILDER ERROR] curvature and torsion are zero\n";
                return result;
            }
        }

        result.curvature =
            kappa;

        result.torsion =
            tau;

        result.angularSpeed =
            tau * op.feedSpeed;

        result.segment =
            PipeCurveSegment::makeHelix(
                op.length,
                kappa,
                tau
            );

        result.valid =
            true;

        return result;
    }

    // Backward-compatible helper.
    // Useful when caller only needs the segment.
    static PipeCurveSegment buildSegment(
        const HelixOperation& op)
    {
        Result result =
            build(op);

        return result.segment;
    }
};