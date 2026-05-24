#pragma once

#include <cmath>

#include "Core/BendDirection.h"
#include "Core/RotationDirection.h"

// =====================================================
// PIPE SEGMENT
//
// Internal geometric representation generated from
// Operation history.
//
// Operation is CNC command.
// PipeSegment is geometric interpretation.
// =====================================================

struct PipeSegment
{
    enum Type
    {
        LINE,
        ARC,
        ROTATE
    };

    Type type = LINE;

    // LINE
    double length = 0.0;

    // ARC
    double curvature = 0.0;
    double angle = 0.0;
    BendDirection bendDirection = BendDirection::CCW;

    // ROTATE
    double rotAngle = 0.0;
    RotationDirection rotationDirection = RotationDirection::CCW;

    double arcLength() const
    {
        if (std::abs(curvature) < 1e-12)
            return 0.0;

        return angle / std::abs(curvature);
    }
};