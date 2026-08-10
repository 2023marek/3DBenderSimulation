#pragma once

#include "Core/Forming/StretchHelixWrappingInput.h"
#include "Core/Forming/StretchHelixWrappingKinematics.h"

// =====================================================
// STRETCH-HELIX WRAPPING KINEMATICS BUILDER
//
// Converts machine motion into target helix geometry.
// =====================================================

class StretchHelixWrappingKinematicsBuilder
{
public:
    static StretchHelixWrappingKinematics build(
        const StretchHelixWrappingInput& input
    );
};