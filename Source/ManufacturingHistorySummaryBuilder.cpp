#include "Core/Forming/ManufacturingHistorySummaryBuilder.h"
#include "Core/Forming/FormingProcessLabels.h"
#include "Core/Forming/PassPlacementLabels.h"
#include <sstream>

namespace
{
   

  

    std::string buildRequestedValueText(
        const AdditionalFormingPass& extra
    )
    {
        std::ostringstream out;

        if (extra.resolvedPlacementMode
            == PassPlacementMode::InsertAtNodeIndex)
        {
            out << "nodeIndex="
                << extra.resolvedNodeIndex;

            return out.str();
        }

        if (extra.resolvedPlacementMode
            == PassPlacementMode::InsertAtArcLength)
        {
            out << "arcLength="
                << extra.requestedArcLength;

            return out.str();
        }

        if (extra.resolvedPlacementMode
            == PassPlacementMode::AppendToPrevious)
        {
            return "end of previous curve";
        }

        if (extra.resolvedPlacementMode
            == PassPlacementMode::ExplicitStartFrame)
        {
            return "explicit frame";
        }

        return "unknown";
    }
}

std::vector<AdditionalPassPlacementSummary>
buildAdditionalPassPlacementSummaries(
    const ManufacturingHistory& history
)
{
    std::vector<AdditionalPassPlacementSummary> summaries;

    for (const AdditionalFormingPass& extra :
        history.additionalPasses)
    {
        AdditionalPassPlacementSummary summary;

        summary.name =
            extra.name;

        summary.processName =
           // processTypeToString(
           //     extra.pass.processType
           // )

            formingProcessTypeToLabel(
                extra.pass.processType
            );

        summary.placementModeName =
            passPlacementModeToLabel(
                extra.resolvedPlacementMode
            );

        summary.resolved =
            extra.hasResolvedPlacement;

        summary.requestedValueText =
            buildRequestedValueText(
                extra
            );

        summary.resolvedArcLength =
            extra.resolvedArcLength;

        summary.entryFrame =
            extra.entryFrame;

        summaries.push_back(
            summary
        );
    }

    return summaries;
}

std::vector<PrimaryPassSummary>
buildPrimaryPassSummaries(
    const ManufacturingHistory& history
)
{
    std::vector<PrimaryPassSummary> summaries;

    for (const ManufacturingPass& pass :
        history.primaryPasses)
    {
        PrimaryPassSummary summary;

        summary.processName =
            formingProcessTypeToLabel(
                pass.processType
            );

        summary.enabled =
            pass.enabled;

        summaries.push_back(
            summary
        );
    }

    return summaries;
}


ManufacturingHistorySummary
buildManufacturingHistorySummary(
    const ManufacturingHistory& history
)
{
    ManufacturingHistorySummary summary;

    summary.primaryPasses =
        buildPrimaryPassSummaries(
            history
        );

    summary.additionalPasses =
        buildAdditionalPassPlacementSummaries(
            history
        );

    return summary;
}