#pragma once

#include <cmath>
static constexpr double PI_VALUE =
3.14159265358979323846;
// =====================================================
// STRETCH-BENDING PIPE SECTION
//
// Geometric properties of a round hollow tube.
//
// Units:
//     outerDiameter      mm
//     wallThickness      mm
//     area               mm^2
//     secondMomentArea   mm^4
//
// This type contains cross-section geometry only.
// It contains no material or machine data.
// =====================================================

struct StretchBendingPipeSection
{
    double outerDiameter =
        0.0;

    double wallThickness =
        0.0;

    bool isValid() const
    {
        if (!std::isfinite(outerDiameter)
            || !std::isfinite(wallThickness))
        {
            return false;
        }

        if (outerDiameter <= 0.0)
            return false;

        if (wallThickness <= 0.0)
            return false;

        if (2.0 * wallThickness
            >= outerDiameter)
        {
            return false;
        }

        return true;
    }

    double innerDiameter() const
    {
        if (!isValid())
            return 0.0;

        return outerDiameter
            - 2.0 * wallThickness;
    }

    double area() const
    {
        if (!isValid())
            return 0.0;

        const double inner =
            innerDiameter();

        return  PI_VALUE / 4.0
            * (
                outerDiameter * outerDiameter
                - inner * inner
                );
    }

    double secondMomentArea() const
    {
        if (!isValid())
            return 0.0;

        const double inner =
            innerDiameter();

        return  PI_VALUE / 64.0
            * (
                std::pow(
                    outerDiameter,
                    4.0
                )
                - std::pow(
                    inner,
                    4.0
                )
                );
    }
};