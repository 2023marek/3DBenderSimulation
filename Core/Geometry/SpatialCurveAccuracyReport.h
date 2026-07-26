#pragma once

// =====================================================
// SPATIAL CURVE ACCURACY REPORT
//
// Contains numerical differences between an integrated
// curve and a known analytical/reference solution.
//
// Used for testing and diagnostics only.
// It does not affect manufacturing state.
// =====================================================

struct SpatialCurveAccuracyReport
{
    double endpointPositionError =
        0.0;

    double endpointTangentError =
        0.0;

    double integratedLengthError =
        0.0;

    double relativePositionError =
        0.0;

    bool positionAccepted =
        false;

    bool tangentAccepted =
        false;

    bool lengthAccepted =
        false;

    bool accepted =
        false;
};