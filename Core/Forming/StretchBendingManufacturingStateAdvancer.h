#pragma once

#include "StretchBendingManufacturingState.h"
#include "StretchBendingManufacturingTiming.h"

// =====================================================
// STRETCH-BENDING MANUFACTURING STATE ADVANCER
//
// Advances an already validated manufacturing state
// through its process stages.
//
// Important architectural boundary:
//
//     evaluator:
//         calculates process feasibility and commands
//
//     profile builders:
//         build loaded and final kappa/tau profiles
//
//     state advancer:
//         advances manufacturing time and stage fractions
//
//     spatial integrator:
//         creates geometric nodes
//
// This class does not alter the rotary-draw process.
// =====================================================

class StretchBendingManufacturingStateAdvancer
{
public:
    static void advance(
        StretchBendingManufacturingState& state,
        double dt,
        const StretchBendingManufacturingTiming& timing
    );

private:
    static double clamp01(
        double value
    );
};