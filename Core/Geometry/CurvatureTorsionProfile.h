#pragma once

#include <vector>

#include "Core/Geometry/CurvatureTorsionSample.h"

// =====================================================
// CURVATURE / TORSION PROFILE
//
// Describes ?(s) and ?(s) along a pipe centerline.
//
// Examples:
//
// Planar two-roller arc:
//     curvature != 0
//     torsion = 0
//
// Circular helix:
//     curvature = constant
//     torsion = constant
//
// Variable-radius process:
//     curvature changes with arc length
// =====================================================

struct CurvatureTorsionProfile
{
    std::vector<CurvatureTorsionSample> samples;

    double totalArcLength =
        0.0;

    bool valid =
        false;

    void clear()
    {
        samples.clear();

        totalArcLength =
            0.0;

        valid =
            false;
    }
};
