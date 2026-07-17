
#include <sstream>
#include "Core/Forming/ManufacturingHistoryBuilder.h"
#include "Core/Forming/PassPlacementResolver.h"
#include "Core/Curve/PipeCurve.h"
#include "Core/Forming/FormingProcessLabels.h"
namespace
{
    std::string additionalPassNameFromProcessType(
        TubeFormingProcessType type,
        size_t additionalIndex
    )
    {
        std::ostringstream out;

        out << "Additional pass "
            << additionalIndex + 1
            << ": "
            << formingProcessTypeToLabel(type);

        return out.str();
    }

    PipeCurve buildBaseCurveBeforePassIndex(
        const ManufacturingPlan& plan,
        size_t targetPassIndex
    )
    {
        PipeCurve baseCurve;

        for (size_t i = 0; i < targetPassIndex; ++i)
        {
            const ManufacturingPass& pass =
                plan.passes[i];

            if (!pass.enabled)
                continue;

            baseCurve.appendCurve(
                pass.outputCurve
            );
        }

        return baseCurve;
    }
}

void buildManufacturingHistoryFromPlan(
    const ManufacturingPlan& plan,
    ManufacturingHistory& history
)
{
    
    history.clear();

    if (!plan.passes.empty())
    {
        history.primaryPasses.push_back(
            plan.passes[0]
        );
    }

    for (size_t passIndex = 1;
        passIndex < plan.passes.size();
        ++passIndex)
    {
        const ManufacturingPass& pass =
            plan.passes[passIndex];

        AdditionalFormingPass extra;
        const size_t additionalIndex =
            passIndex - 1;

        extra.name =
            additionalPassNameFromProcessType(
                pass.processType,
                additionalIndex
            );

        extra.pass =
            pass;

        PipeCurve baseCurve =
            buildBaseCurveBeforePassIndex(
                plan,
                passIndex
            );

        PassPlacementResolution placementResult =
            resolvePassPlacementFrame(
                pass.placement,
                baseCurve,
                0.5
            );
        extra.resolvedPlacementMode =
            pass.placement.mode;

        extra.resolvedNodeIndex =
            pass.placement.nodeIndex;

        extra.requestedArcLength =
            pass.placement.arcLength;

        if (placementResult.valid)
        {
            extra.entryFrame =
                placementResult.frame;
            extra.deformableRegion =
                pass.deformableRegion;

            extra.resolvedArcLength =
                placementResult.arcLength;

            extra.hasResolvedPlacement =
                true;
        }
        else
        {

            extra.hasResolvedPlacement =
            false;
        
            extra.entryFrame.P = { 0.0, 0.0, 0.0 };
            extra.entryFrame.T = { 1.0, 0.0, 0.0 };
            extra.entryFrame.N = { 0.0, 1.0, 0.0 };
            extra.entryFrame.B = { 0.0, 0.0, 1.0 };
        }

        history.additionalPasses.push_back(
            extra
        );
    }
}