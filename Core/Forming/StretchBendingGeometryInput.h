#pragma once

#include <cmath>

// =====================================================
// STRETCH-BENDING GEOMETRY INPUT
//
// Target centerline geometry for one stretch-bending
// operation.
//
// The first implementation supports constant:
//
//     curvature ?
//     torsion ?
//
// over a requested material centerline length.
//
// Later this may be replaced or extended by a full
// CurvatureTorsionProfile.
// =====================================================

struct StretchBendingGeometryInput
{
    double targetArcLength =
        0.0;

    double targetCurvature =
        0.0;

    double targetTorsion =
        0.0;

    // Fraction of loaded curvature expected to recover
// during unloading.
//
// Example:
//
//     0.10 = 10% curvature recovery
//
// Valid range:
//
//     0 <= springbackRatio < 1
   
   
    bool isValid() const
    {
        if (!std::isfinite(targetArcLength)
            || !std::isfinite(targetCurvature)
            || !std::isfinite(targetTorsion))
        {
            return false;
        }

        if (targetArcLength <= 0.0)
            return false;

        if (targetCurvature < 0.0)
            return false;

        
       

        return true;
    }
};