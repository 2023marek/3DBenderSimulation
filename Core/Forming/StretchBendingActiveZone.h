#pragma once

#include <cmath>

// =====================================================
// STRETCH-BENDING ACTIVE ZONE
//
// Defines the part of the pipe where bending deformation
// is currently allowed to occur.
//
// Coordinates use pipe arc length:
//
//     s = 0              pipe start
//     s = totalLength    pipe end
//
// Phase 10J uses a fixed zone.
//
// Later phases may introduce:
//
//     moving active zones
//     growing active zones
//     machine-relative active zones
// =====================================================

struct StretchBendingActiveZone
{
    // -------------------------------------------------
    // Arc-length coordinate where the active zone begins.
    // -------------------------------------------------

    double startS = 0.0;

    // -------------------------------------------------
    // Arc-length coordinate where the active zone ends.
    // -------------------------------------------------

    double endS = 0.0;

    // -------------------------------------------------
    // Returns the zone length.
    // -------------------------------------------------

    double length() const
    {
        return endS - startS;
    }

    // -------------------------------------------------
    // Basic mathematical validity.
    //
    // This function does not know the pipe length.
    // Full validation against the pipe belongs in
    // isValidForLength().
    // -------------------------------------------------

    bool isValid() const
    {
        return
            std::isfinite(startS)
            && std::isfinite(endS)
            && startS >= 0.0
            && endS > startS;
    }

    // -------------------------------------------------
    // Checks whether this zone fits inside a pipe with
    // the supplied total arc length.
    // -------------------------------------------------

    bool isValidForLength(
        double totalLength
    ) const
    {
        return
            isValid()
            && std::isfinite(totalLength)
            && totalLength > 0.0
            && endS <= totalLength;
    }

    // -------------------------------------------------
    // Returns true when the supplied pipe coordinate lies
    // inside the active zone.
    //
    // Inclusive boundaries are useful for rendering and
    // sample classification.
    // -------------------------------------------------

    bool contains(
        double s
    ) const
    {
        return
            isValid()
            && std::isfinite(s)
            && s >= startS
            && s <= endS;
    }
};
