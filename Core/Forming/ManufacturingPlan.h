#pragma once

#include <vector>
#include <iostream>

#include "Core/Forming/ManufacturingPass.h"
#include "Core/Curve/PipeCurve.h"
#include "Core/Forming/PassPlacement.h"

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
        // Supported now:
        //
        // AppendToPrevious:
        //     add pass curve to the end.
        //
        // InsertAtArcLength:
        //     split current combined curve at arc length s,
        //     insert pass curve there,
        //     append remaining curve after it.
        //
        // Current limitation:
        //     splitAtArcLength currently splits Line segments exactly.
        //     Arc / helix split support comes later.
        // =====================================================

        PipeCurve combined;

        for (const auto& pass : passes)
        {
            if (!pass.enabled)
                continue;

            if (pass.placement.mode
                == PassPlacementMode::AppendToPrevious)
            {
                combined.appendCurve(
                    pass.outputCurve
                );
            }
            else if (pass.placement.mode
                == PassPlacementMode::InsertAtArcLength)
            {
                PipeCurveSplitResult split =
                    combined.splitAtArcLength(
                        pass.placement.arcLength
                    );

                if (!split.valid)
                {
                    std::cerr
                        << "[PLAN COMPOSE WARNING] Invalid insert arc length. Appending pass instead.\n";

                    combined.appendCurve(
                        pass.outputCurve
                    );

                    continue;
                }

                PipeCurve rebuilt;

                rebuilt.appendCurve(
                    split.before
                );

                rebuilt.appendCurve(
                    pass.outputCurve
                );

                rebuilt.appendCurve(
                    split.after
                );

                combined =
                    rebuilt;

                std::cout
                    << "[PLAN COMPOSE INSERT] arcLength="
                    << pass.placement.arcLength
                    << " beforeSegments="
                    << split.before.size()
                    << " insertedSegments="
                    << pass.outputCurve.size()
                    << " afterSegments="
                    << split.after.size()
                    << " combinedSegments="
                    << combined.size()
                    << std::endl;
            }
            else
            {
                // =================================================
                // Future:
                // InsertAtNodeIndex
                // ExplicitStartFrame
                //
                // Safe fallback for now.
                // =================================================

                combined.appendCurve(
                    pass.outputCurve
                );
            }
        }

        return combined;
    }
};