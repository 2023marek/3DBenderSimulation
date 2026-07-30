#pragma once

#include <cmath>

#include "StretchBendingManufacturingStage.h"
#include "StretchBendingActiveZone.h"

// =====================================================
// STRETCH-BENDING MANUFACTURING STATE
//
// Stores the current runtime state of one independent
// stretch-bending manufacturing process.
//
// This structure contains state data only.
//
// It does not:
//
//     evaluate process feasibility
//     build curvature/torsion profiles
//     integrate pipe geometry
//     render the pipe
//     advance itself through time
//
// Responsibilities:
//
//     StateBuilder
//         creates a validated Ready state
//
//     StateAdvancer
//         changes this state over time
//
//     ManufacturingState
//         stores and validates the current values
// =====================================================

struct StretchBendingManufacturingState
{
    // -------------------------------------------------
    // Current manufacturing stage.
    //
    // A default-created or cleared state is Invalid.
    // -------------------------------------------------

    StretchBendingManufacturingStage stage =
        StretchBendingManufacturingStage::Invalid;

    // -------------------------------------------------
    // Time elapsed since the manufacturing sequence
    // started.
    //
    // Unit:
    //     seconds
    // -------------------------------------------------

    double elapsedTime =
        0.0;

    // -------------------------------------------------
    // Overall normalized process progress.
    //
    // Range:
    //     0.0 = process not started
    //     1.0 = process complete
    // -------------------------------------------------

    double processProgress =
        0.0;

    // -------------------------------------------------
    // Applied axial-tension fraction.
    //
    // Range:
    //     0.0 = no tension
    //     1.0 = full commanded tension
    // -------------------------------------------------

    double tensionFraction =
        0.0;

    // -------------------------------------------------
    // Applied bending-command fraction.
    //
    // Range:
    //     0.0 = no bending command
    //     1.0 = full loaded bending command
    // -------------------------------------------------

    double bendingFraction =
        0.0;

    // -------------------------------------------------
    // Unloading progress.
    //
    // Range:
    //     0.0 = unloading has not started
    //     1.0 = unloading is complete
    // -------------------------------------------------

    double unloadingFraction =
        0.0;

    // -------------------------------------------------
    // Material region affected by this stretch-bending
    // operation.
    // -------------------------------------------------

    StretchBendingActiveZone activeZone;

    // -------------------------------------------------
    // General validity flag.
    //
    // The builder sets this to true only after all
    // required input and active-zone checks succeed.
    // -------------------------------------------------

    bool valid =
        false;

    // =================================================
    // CLEAR
    //
    // Restores the same values as a newly constructed
    // invalid state.
    // =================================================

    void clear()
    {
        stage =
            StretchBendingManufacturingStage::Invalid;

        elapsedTime =
            0.0;

        processProgress =
            0.0;

        tensionFraction =
            0.0;

        bendingFraction =
            0.0;

        unloadingFraction =
            0.0;

        activeZone =
            StretchBendingActiveZone{};

        valid =
            false;
    }

    // =================================================
    // BASIC VALIDITY
    //
    // Checks values that do not require knowledge of
    // the complete pipe length.
    // =================================================

    bool isValid() const
    {
        if (!valid)
        {
            return false;
        }

        if (stage
            == StretchBendingManufacturingStage::Invalid)
        {
            return false;
        }

        if (!std::isfinite(elapsedTime)
            || elapsedTime < 0.0)
        {
            return false;
        }

        if (!isUnitFraction(processProgress))
        {
            return false;
        }

        if (!isUnitFraction(tensionFraction))
        {
            return false;
        }

        if (!isUnitFraction(bendingFraction))
        {
            return false;
        }

        if (!isUnitFraction(unloadingFraction))
        {
            return false;
        }

        return true;
    }

    // =================================================
    // PIPE-LENGTH-AWARE VALIDITY
    //
    // Performs all basic state checks and also verifies
    // that the active zone lies inside the pipe.
    // =================================================

    bool isValidForLength(
        double totalLength) const
    {
        if (!isValid())
        {
            return false;
        }

        if (!std::isfinite(totalLength)
            || totalLength <= 0.0)
        {
            return false;
        }

        if (!activeZone.isValidForLength(
            totalLength
        ))
        {
            return false;
        }

        return true;
    }

private:

    // =================================================
    // NORMALIZED-FRACTION CHECK
    // =================================================

    static bool isUnitFraction(
        double value)
    {
        return std::isfinite(value)
            && value >= 0.0
            && value <= 1.0;
    }
};