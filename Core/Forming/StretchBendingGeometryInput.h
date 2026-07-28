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