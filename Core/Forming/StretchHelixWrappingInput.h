#pragma once

#include <cmath>

#include "Core/Forming/StretchBendingPipeSection.h"
#include "Core/Forming/StretchBendingMaterial.h"

// =====================================================
// STRETCH-HELIX WRAPPING INPUT
//
// Describes the commanded setup for progressively
// wrapping a tensioned pipe around a cylindrical support.
//
// This structure contains only process input.
// It does NOT contain:
//
//     current contact state
//     current wrapped length
//     generated geometry
//     springback result
//
// Those belong to later phases.
// =====================================================

struct StretchHelixWrappingInput
{
    // =================================================
    // WORKPIECE
    // =================================================

    StretchBendingPipeSection pipeSection;

    StretchBendingMaterial material;

    // Total centerline length participating in this
    // standalone wrapping process.
    //
    // Units:
    //     mm
    double pipeArcLength =
        0.0;

    // =================================================
    // CYLINDRICAL SUPPORT
    // =================================================

    // Outside radius of the cylindrical support around
    // which the workpiece is wrapped.
    //
    // IMPORTANT:
    //
    // This is NOT the workpiece centerline helix radius.
    //
    // Units:
    //     mm
    double supportOuterRadius =
        0.0;

    // =================================================
    // MACHINE KINEMATICS
    // =================================================

    // Axial translation speed along the support axis.
    //
    // Units:
    //     mm / s
    double axialSpeed =
        0.0;

    // Angular rotation speed around the support axis.
    //
    // Units:
    //     rad / s
    double rotationSpeed =
        0.0;

    // Rotation direction.
    //
    // +1 = one handedness
    // -1 = opposite handedness
    int rotationDirection =
        1;

    // =================================================
    // STRETCH COMMAND
    // =================================================

    // Axial centerline strain applied to the workpiece.
    double axialStretchStrain =
        0.0;

    // =================================================
    // NUMERICAL SETTINGS
    // =================================================

    // Spatial integration step.
    //
    // Units:
    //     mm
    double sampleStep =
        0.25;

    bool enabled =
        true;

    // =================================================
    // VALIDATION
    // =================================================

    bool isValid() const
    {
        if (!enabled)
            return false;

        if (!pipeSection.isValid())
            return false;

        if (!material.isValid())
            return false;

        if (!std::isfinite(pipeArcLength)
            || !std::isfinite(supportOuterRadius)
            || !std::isfinite(axialSpeed)
            || !std::isfinite(rotationSpeed)
            || !std::isfinite(axialStretchStrain)
            || !std::isfinite(sampleStep))
        {
            return false;
        }

        if (pipeArcLength <= 0.0)
            return false;

        if (supportOuterRadius <= 0.0)
            return false;

        if (axialSpeed < 0.0)
            return false;

        if (std::abs(rotationSpeed) <= 1e-12)
            return false;

        if (rotationDirection != 1
            && rotationDirection != -1)
        {
            return false;
        }

        if (axialStretchStrain < 0.0)
            return false;

        if (sampleStep <= 1e-9)
            return false;

        return true;
    }
};