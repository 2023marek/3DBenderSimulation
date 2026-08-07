#pragma once

#include <cmath>

#include "Core/Forming/StretchBendingPipeSection.h"
#include "Core/Forming/StretchBendingMaterial.h"

// =====================================================
// STRETCH-BENDING OPERATION
//
// Reusable description of one requested stretch-bending
// manufacturing operation.
//
// Unlike StretchBendingProcessInput, this object is
// suitable for storage in a manufacturing plan.
//
// It describes what should be manufactured, not the
// current runtime state.
// =====================================================

struct StretchBendingOperation
{
    // Pipe cross-section used by this operation.
    StretchBendingPipeSection pipeSection;

    // Material model used for feasibility and force
    // evaluation.
    StretchBendingMaterial material;

    // Requested final curvature after unloading.
    //
    // Units:
    //     1 / mm
    double targetFinalCurvature =
        0.0;

    // Requested torsion.
    //
    // Units:
    //     1 / mm
    double targetTorsion =
        0.0;

    // Total pipe-centerline length represented by this
    // standalone operation.
    //
    // Units:
    //     mm
    double arcLength =
        0.0;

    // Commanded axial centerline strain.
    double axialStretchStrain =
        0.0;

    // Material feed speed.
    //
    // Units:
    //     mm / second
    double feedSpeed =
        0.0;

    // Requested springback compensation.
    bool compensateSpringback =
        true;

    // Fraction of loaded curvature expected to recover.
    //
    // Must satisfy:
    //
    //     0 <= springbackRatio < 1
    double springbackRatio =
        0.0;

    // Spatial integration sample distance.
    //
    // Units:
    //     mm
    double sampleStep =
        0.5;

    bool enabled =
        true;

    bool isValid() const
    {
        if (!enabled)
            return false;

        if (!pipeSection.isValid())
            return false;

        if (!material.isValid())
            return false;

        if (!std::isfinite(targetFinalCurvature)
            || !std::isfinite(targetTorsion)
            || !std::isfinite(arcLength)
            || !std::isfinite(axialStretchStrain)
            || !std::isfinite(feedSpeed)
            || !std::isfinite(springbackRatio)
            || !std::isfinite(sampleStep))
        {
            return false;
        }

        if (targetFinalCurvature < 0.0)
            return false;

        if (arcLength <= 0.0)
            return false;

        if (axialStretchStrain < 0.0)
            return false;

        if (feedSpeed <= 0.0)
            return false;

        if (springbackRatio < 0.0
            || springbackRatio >= 1.0)
        {
            return false;
        }

        if (sampleStep <= 1e-9)
            return false;

        return true;
    }
};