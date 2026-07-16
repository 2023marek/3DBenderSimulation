#pragma once

// =====================================================
// DEFORMABLE REGION
//
// Describes the physical range of an already formed pipe
// that may be modified by an additional forming pass.
//
// Arc-length coordinates are measured along the current
// manufactured pipe geometry.
//
// startArcLength:
//     physical location where deformation begins.
//
// endArcLength:
//     physical location where deformation ends.
//
// No geometry modification happens in this type.
// It is data only.
// =====================================================

struct DeformableRegion
{
    double startArcLength =
        0.0;

    double endArcLength =
        0.0;

    bool isValid() const
    {
        return startArcLength >= 0.0
            && endArcLength > startArcLength;
    }

    double length() const
    {
        if (!isValid())
            return 0.0;

        return endArcLength
            - startArcLength;
    }
};
