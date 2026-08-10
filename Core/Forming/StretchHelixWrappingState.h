#pragma once

#include <cmath>

// =====================================================
// STRETCH-HELIX WRAPPING STATE
//
// Describes how much of the workpiece has progressively
// occupied the reference helix.
//
// This is a kinematic contact state.
//
// It does NOT calculate:
//     contact pressure
//     friction
//     springback
//     plasticity
//
// Those come later.
// =====================================================

struct StretchHelixWrappingState
{
    // Material arc length already wrapped onto the
    // reference helix.
    //
    // Units:
    //     mm
    double wrappedLength =
        0.0;

    // Moving boundary between:
    //
    //     wrapped helix
    //
    // and
    //
    //     unwrapped/free pipe
    //
    // For H4:
    //
    //     contactFrontS = wrappedLength
    double contactFrontS =
        0.0;

    // Normalized:
    //
    //     0 = no wrapping
    //     1 = whole requested pipe wrapped
    double progress =
        0.0;

    bool complete =
        false;

    bool valid =
        false;

    void clear()
    {
        wrappedLength =
            0.0;

        contactFrontS =
            0.0;

        progress =
            0.0;

        complete =
            false;

        valid =
            false;
    }

    bool isValidForLength(
        double totalLength
    ) const
    {
        if (!valid)
            return false;

        if (!std::isfinite(totalLength)
            || totalLength <= 0.0)
        {
            return false;
        }

        if (!std::isfinite(wrappedLength)
            || !std::isfinite(contactFrontS)
            || !std::isfinite(progress))
        {
            return false;
        }

        if (wrappedLength < 0.0
            || wrappedLength > totalLength)
        {
            return false;
        }

        if (contactFrontS < 0.0
            || contactFrontS > totalLength)
        {
            return false;
        }

        if (progress < 0.0
            || progress > 1.0)
        {
            return false;
        }

        return true;
    }
};