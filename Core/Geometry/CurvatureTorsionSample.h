#pragma once

// =====================================================
// CURVATURE / TORSION SAMPLE
//
// Process-independent geometric command for one point
// or interval along a manufactured pipe centerline.
//
// curvature:
//     magnitude of centerline bending, ? = 1 / R.
//
// torsion:
//     spatial twisting rate of the centerline frame.
//
// arcLength:
//     cumulative material distance along the generated
//     centerline.
//
// This type contains geometry instructions only.
// It does not contain machine or material physics.
// =====================================================

struct CurvatureTorsionSample
{
    double arcLength =
        0.0;

    double curvature =
        0.0;

    double torsion =
        0.0;

    bool isValid() const
    {
        return arcLength >= 0.0;
    }
};