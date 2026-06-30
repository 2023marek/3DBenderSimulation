#include "Core/Forming/ManufacturingHistorySummaryBuilder.h"

#include <sstream>

namespace
{
    const char* processTypeToString(
        TubeFormingProcessType type
    )
    {
        switch (type)
        {
        case TubeFormingProcessType::RotaryDrawBending:
            return "RotaryDraw";

        case TubeFormingProcessType::HelixForming:
            return "Helix";

        case TubeFormingProcessType::TwoRollerContinuous:
            return "TwoRollerContinuous";

        case TubeFormingProcessType::StretchBending:
            return "StretchBending";

        default:
            return "Unknown";
        }
    }

    const char* placementModeToString(
        PassPlacementMode mode
    )
    {
        switch (mode)
        {
        case PassPlacementMode::AppendToPrevious:
            return "AppendToPrevious";

        case PassPlacementMode::InsertAtArcLength:
            return "InsertAtArcLength";

        case PassPlacementMode::InsertAtNodeIndex:
            return "InsertAtNodeIndex";

        case PassPlacementMode::ExplicitStartFrame:
            return "ExplicitStartFrame";

        default:
            return "Unknown";
        }
    }

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
            processTypeToString(
                extra.pass.processType
            );

        summary.placementModeName =
            placementModeToString(
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