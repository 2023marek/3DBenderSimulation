#include "StretchBendingCurrentProfileParameterResolver.h"

#include <algorithm>

// =====================================================
// CLAMP NORMALIZED FRACTION
// =====================================================

double
StretchBendingCurrentProfileParameterResolver::clamp01(
    double value)
{
    return std::max(
        0.0,
        std::min(
            1.0,
            value
        )
    );
}

// =====================================================
// LINEAR INTERPOLATION
//
// fraction = 0:
//     returns startValue
//
// fraction = 1:
//     returns endValue
// =====================================================

double
StretchBendingCurrentProfileParameterResolver::interpolate(
    double startValue,
    double endValue,
    double fraction)
{
    const double acceptedFraction =
        clamp01(
            fraction
        );

    return
        startValue
        +
        (
            endValue
            - startValue
            )
        * acceptedFraction;
}

// =====================================================
// RESOLVE CURRENT MATERIAL PARAMETERS
// =====================================================

StretchBendingCurrentProfileParameters
StretchBendingCurrentProfileParameterResolver::resolve(
    const StretchBendingManufacturingState& state,
    const StretchBendingEvaluationResult& evaluation)
{
    StretchBendingCurrentProfileParameters parameters;

    // -------------------------------------------------
    // Reject invalid inputs.
    // -------------------------------------------------

    if (!state.isValid())
    {
        return parameters;
    }

    if (!evaluation.valid)
    {
        return parameters;
    }

    if (!evaluation.springbackPredictionValid)
    {
        return parameters;
    }
   

    // -------------------------------------------------
    // Current torsion behavior
    //
    // Torsional loading and torsional springback are not
    // modeled yet.
    //
    // Therefore torsion passes through unchanged from
    // the evaluated process result.
    // -------------------------------------------------

    parameters.torsion =
        evaluation.targetTorsion;

    // =================================================
    // READY
    //
    // Pipe is prepared, but no forming load exists.
    // =================================================

    if (state.stage
        == StretchBendingManufacturingStage::Ready)
    {
        parameters.curvature =
            0.0;

        parameters.valid =
            true;

        return parameters;
    }

    // =================================================
    // APPLYING TENSION
    //
    // Axial tension alone does not create centerline
    // curvature in the current simplified model.
    // =================================================

    if (state.stage
        == StretchBendingManufacturingStage::
        ApplyingTension)
    {
        parameters.curvature =
            0.0;

        parameters.valid =
            true;

        return parameters;
    }

    // =================================================
    // FORMING
    //
    // Loaded curvature is introduced according to the
    // normalized bending fraction.
    //
    // kappaCurrent =
    //     bendingFraction * kappaLoaded
    // =================================================

    if (state.stage
        == StretchBendingManufacturingStage::Forming)
    {
        parameters.curvature =
            evaluation.loadedCurvatureCommand
            * clamp01(
                state.bendingFraction
            );

        parameters.valid =
            true;

        return parameters;
    }

    // =================================================
    // LOADED HOLD
    //
    // Full machine-loaded curvature is maintained.
    // =================================================

    if (state.stage
        == StretchBendingManufacturingStage::LoadedHold)
    {
        parameters.curvature =
            evaluation.loadedCurvatureCommand;

        parameters.valid =
            true;

        return parameters;
    }

    // =================================================
    // UNLOADING
    //
    // Curvature transitions from the loaded value to
    // the predicted final value.
    //
    // kappaCurrent =
    //
    //     (1-u) * kappaLoaded
    //     +
    //     u * kappaFinal
    //
    // where:
    //
    //     u = unloadingFraction
    // =================================================

    if (state.stage
        == StretchBendingManufacturingStage::Unloading)
    {
        parameters.curvature =
            interpolate(
                evaluation.loadedCurvatureCommand,
                evaluation.predictedFinalCurvature,
                state.unloadingFraction
            );

        parameters.valid =
            true;

        return parameters;
    }

    // =================================================
    // COMPLETE
    //
    // Only predicted permanent curvature remains.
    // =================================================

    if (state.stage
        == StretchBendingManufacturingStage::Complete)
    {
        parameters.curvature =
            evaluation.predictedFinalCurvature;

        parameters.valid =
            true;

        return parameters;
    }

    // Invalid or unsupported stage returns the default
    // invalid parameter structure.
    return parameters;
}