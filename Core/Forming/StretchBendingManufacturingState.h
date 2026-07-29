#pragma once

#include <cmath>

#include "Core/Forming/StretchBendingManufacturingStage.h"
#include "Core/Forming/StretchBendingActiveZone.h"

// =====================================================
// STRETCH-BENDING MANUFACTURING STATE
//
// Represents one snapshot of the stretch-bending process.
//
// It does not own geometry.
//
// It describes:
//
//     process stage
//     overall progress
//     active forming zone
//     applied tension fraction
//     applied bending fraction
//     unloading fraction
//
// Geometry generation and rendering remain separate.
// =====================================================

struct StretchBendingManufacturingState
{
    // -------------------------------------------------
    // Current high-level manufacturing stage.
    // -------------------------------------------------

    StretchBendingManufacturingStage stage =
        StretchBendingManufacturingStage::Invalid;

    // -------------------------------------------------
    // Overall normalized playback progress.
    //
    // Expected range:
    //
    //     0.0 = process beginning
    //     1.0 = process complete
    // -------------------------------------------------

    double processProgress = 0.0;

    // -------------------------------------------------
    // Fixed pipe region where bending deformation occurs.
    // -------------------------------------------------

    StretchBendingActiveZone activeZone;

    // -------------------------------------------------
    // Fraction of commanded axial tension currently
    // applied.
    //
    //     0.0 = no tension
    //     1.0 = full commanded tension
    // -------------------------------------------------

    double tensionFraction = 0.0;

    // -------------------------------------------------
    // Fraction of loaded bending command currently
    // applied.
    //
    //     0.0 = straight/unbent state
    //     1.0 = full loaded curvature
    // -------------------------------------------------

    double bendingFraction = 0.0;

    // -------------------------------------------------
    // Fraction of unloading completed.
    //
    //     0.0 = loaded shape
    //     1.0 = final unloaded shape
    //
    // This remains zero before the Unloading stage.
    // -------------------------------------------------

    double unloadingFraction = 0.0;

    // -------------------------------------------------
    // Returns true if all scalar values are finite and
    // inside their normalized ranges.
    // -------------------------------------------------

    bool isValidForLength(
        double totalLength
    ) const
    {
        if (stage
            == StretchBendingManufacturingStage::Invalid)
        {
            return false;
        }

        if (!std::isfinite(processProgress)
            || !std::isfinite(tensionFraction)
            || !std::isfinite(bendingFraction)
            || !std::isfinite(unloadingFraction))
        {
            return false;
        }

        if (processProgress < 0.0
            || processProgress > 1.0)
        {
            return false;
        }

        if (tensionFraction < 0.0
            || tensionFraction > 1.0)
        {
            return false;
        }

        if (bendingFraction < 0.0
            || bendingFraction > 1.0)
        {
            return false;
        }

        if (unloadingFraction < 0.0
            || unloadingFraction > 1.0)
        {
            return false;
        }

        return activeZone.isValidForLength(
            totalLength
        );
    }
};
