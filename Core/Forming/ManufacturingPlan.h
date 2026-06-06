#pragma once

#include <vector>

#include "Core/Forming/ManufacturingPass.h"
#include "Core/Curve/PipeCurve.h"

// =====================================================
// MANUFACTURING PLAN
//
// Ordered list of manufacturing passes.
//
// Future examples:
//
// Pass 1: rotary draw bending
// Pass 2: helix forming
// Pass 3: manual correction
// =====================================================

struct ManufacturingPlan
{
    std::vector<ManufacturingPass> passes;

    void clear()
    {
        passes.clear();
    }

    bool empty() const
    {
        return passes.empty();
    }

    size_t size() const
    {
        return passes.size();
    }

    void addPass(const ManufacturingPass& pass)
    {
        passes.push_back(pass);
    }

    PipeCurve buildCombinedCurve() const
    {
        // =====================================================
        // MULTI-PASS CURVE COMPOSITION
        //
        // Each pass owns an outputCurve.
        //
        // This does not resimulate passes.
        // It only concatenates existing curve outputs.
        // =====================================================

        PipeCurve combined;

        for (const auto& pass : passes)
        {
            if (!pass.enabled)
                continue;

            combined.appendCurve(
                pass.outputCurve
            );
        }

        return combined;
    }
};