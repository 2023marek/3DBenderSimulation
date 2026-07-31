#pragma once

#include <cmath>

// =====================================================
// STRETCH-BENDING CURRENT PROFILE PARAMETERS
//
// Stores the instantaneous curvature and torsion values
// calculated for the current manufacturing state.
//
// This structure does not:
//
//     advance manufacturing time
//     integrate geometry
//     build the complete spatial profile
//     evaluate feasibility
//
// It represents the material command that will later be
// converted into a CurvatureTorsionProfile.
// =====================================================

struct StretchBendingCurrentProfileParameters
{
    // -------------------------------------------------
    // Instantaneous curvature command.
    //
    // Unit:
    //     1 / length
    //
    // Example:
    //     0.002 1/mm
    // -------------------------------------------------

    double curvature =
        0.0;

    // -------------------------------------------------
    // Instantaneous torsion command.
    //
    // Unit:
    //     1 / length
    // -------------------------------------------------

    double torsion =
        0.0;

    // -------------------------------------------------
    // Indicates whether these values were calculated
    // from a valid manufacturing state and evaluation.
    // -------------------------------------------------

    bool valid =
        false;

    // =================================================
    // CLEAR
    // =================================================

    void clear()
    {
        curvature =
            0.0;

        torsion =
            0.0;

        valid =
            false;
    }

    // =================================================
    // VALIDITY
    // =================================================

    bool isValid() const
    {
        if (!valid)
        {
            return false;
        }

        if (!std::isfinite(curvature))
        {
            return false;
        }

        if (!std::isfinite(torsion))
        {
            return false;
        }

        // Current implementation supports non-negative
        // bending curvature.
        //
        // Signed curvature can be introduced later if
        // bend-direction semantics require it.
        if (curvature < 0.0)
        {
            return false;
        }

        return true;
    }
};