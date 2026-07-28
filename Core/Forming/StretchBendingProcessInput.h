#pragma once

#include <cmath>

#include "Core/Forming/StretchBendingPipeSection.h"
#include "Core/Forming/StretchBendingMaterial.h"
#include "Core/Forming/StretchBendingGeometryInput.h"

// =====================================================
// STRETCH-BENDING PROCESS INPUT
//
// Complete input required to validate and later execute
// one stretch-bending operation.
//
// This is a process command.
// It does not contain runtime manufacturing state.
// =====================================================

struct StretchBendingProcessInput
{
    StretchBendingPipeSection pipeSection;

    StretchBendingMaterial material;

    StretchBendingGeometryInput geometry;

    // Axial centerline strain applied by stretching.
    //
    // epsilon_0 = T / (E A)
    double axialStretchStrain =
        0.0;

    // Material feed speed through the fixed active zone.
    //
    // Units:
    //     mm / second
    double feedSpeed =
        0.0;

    // Sampling distance passed to the shared spatial
    // curve integrator.
    double sampleStep =
        0.5;

   

    double springbackRatio =
        0.0;

    bool compensateSpringback =
        true;

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

     if (!geometry.isValid())
         return false;

     if (!std::isfinite(axialStretchStrain)
         || !std::isfinite(feedSpeed)
         || !std::isfinite(sampleStep)
         || !std::isfinite(springbackRatio))
     {
         return false;
     }

     if (axialStretchStrain < 0.0)
         return false;

     if (feedSpeed <= 0.0)
         return false;

     if (sampleStep <= 1e-9)
         return false;

     if (springbackRatio < 0.0
         || springbackRatio >= 1.0)
     {
         return false;
     }

     return true;
 }
};