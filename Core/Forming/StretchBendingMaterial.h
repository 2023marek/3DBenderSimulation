#pragma once

#include <cmath>

// =====================================================
// STRETCH-BENDING MATERIAL
//
// First material model for stretch-bending validation.
//
// Units:
//     youngModulus       force / mm^2
//     yieldStress        force / mm^2
//     hardeningModulus   force / mm^2
//
// Strains are dimensionless.
//
// This type stores material properties only.
// It does not compute geometry or machine motion.
// =====================================================

struct StretchBendingMaterial
{
    double youngModulus =
        0.0;

    double yieldStress =
        0.0;

    double hardeningModulus =
        0.0;

    double allowableStrain =
        0.0;

    bool isValid() const
    {
        if (!std::isfinite(youngModulus)
            || !std::isfinite(yieldStress)
            || !std::isfinite(hardeningModulus)
            || !std::isfinite(allowableStrain))
        {
            return false;
        }

        if (youngModulus <= 0.0)
            return false;

        if (yieldStress <= 0.0)
            return false;

        if (hardeningModulus < 0.0)
            return false;

        if (allowableStrain <= 0.0)
            return false;

        return true;
    }

    double yieldStrain() const
    {
        if (!isValid())
            return 0.0;

        return yieldStress
            / youngModulus;
    }
};