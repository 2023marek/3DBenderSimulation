#pragma once

#include "Core/Geometry/CurvatureTorsionProfile.h"

// =====================================================
// CONSTANT CURVATURE / TORSION PROFILE BUILDER
//
// Creates a process-independent profile with constant:
//
//     curvature ?
//     torsion ?
//
// along a requested centerline arc length.
//
// Example uses:
//
//     Straight:
//         ? = 0
//         ? = 0
//
//     Planar circular arc:
//         ? != 0
//         ? = 0
//
//     Circular helix:
//         ? != 0
//         ? != 0
//
// This builder creates geometry commands only.
// It does not perform integration or manufacturing.
// =====================================================

class ConstantCurvatureTorsionProfileBuilder
{
public:
    static CurvatureTorsionProfile build(
        double totalArcLength,
        double curvature,
        double torsion
    );
};
