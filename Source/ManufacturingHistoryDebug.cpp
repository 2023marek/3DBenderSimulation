#include "Core/Forming/ManufacturingHistoryDebug.h"
#include "Core/Math/Vec3D.h"
#include "Core/Forming/ManufacturingHistorySummaryBuilder.h"
#include <iostream>

namespace
{
    const char* manufacturingProcessTypeToString(
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


    void debugPrintVec3(
        const char* label,
        const Vec3D& v
    )
    {
        std::cout << "      " << label << "=("
            << v.x << ", "
            << v.y << ", "
            << v.z << ")"
            << std::endl;
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


    void debugPrintPlacementDetails(
        const AdditionalFormingPass& extra
    )
    {
        std::cout << "      placementMode="
            << placementModeToString(
                extra.resolvedPlacementMode
            )
            << " resolved="
            << extra.hasResolvedPlacement
            << std::endl;

        if (extra.resolvedPlacementMode
            == PassPlacementMode::InsertAtNodeIndex)
        {
            std::cout << "      requestedNodeIndex="
                << extra.resolvedNodeIndex
                << std::endl;

            std::cout << "      resolvedArcLength="
                << extra.resolvedArcLength
                << std::endl;

            return;
        }

        if (extra.resolvedPlacementMode
            == PassPlacementMode::InsertAtArcLength)
        {
            std::cout << "      requestedArcLength="
                << extra.requestedArcLength
                << std::endl;

            std::cout << "      resolvedArcLength="
                << extra.resolvedArcLength
                << std::endl;

            return;
        }

        if (extra.resolvedPlacementMode
            == PassPlacementMode::AppendToPrevious)
        {
            std::cout << "      resolvedArcLength="
                << extra.resolvedArcLength
                << " (end of previous curve)"
                << std::endl;

            return;
        }

        if (extra.resolvedPlacementMode
            == PassPlacementMode::ExplicitStartFrame)
        {
            std::cout << "      explicit start frame"
                << std::endl;

            return;
        }
    }
}

void debugPrintManufacturingHistory(
    const ManufacturingHistory& history
)
{
    std::cout << "[MFG HISTORY]" << std::endl;

    std::cout << "  primary passes: "
        << history.primaryPasses.size()
        << std::endl;

    for (size_t i = 0; i < history.primaryPasses.size(); ++i)
    {
        const ManufacturingPass& pass =
            history.primaryPasses[i];

        std::cout << "    [" << i << "] process="
            << manufacturingProcessTypeToString(pass.processType)
            << std::endl;
    }

    auto additionalSummaries =
        buildAdditionalPassPlacementSummaries(
            history
        );

    std::cout << "  additional passes: "
        << additionalSummaries.size()
        << std::endl;

    for (size_t i = 0; i < additionalSummaries.size(); ++i)
    {
        const AdditionalPassPlacementSummary& summary =
            additionalSummaries[i];

        std::cout << "    [" << i << "] name="
            << summary.name
            << " process="
            << summary.processName
            << std::endl;

        std::cout << "      placementMode="
            << summary.placementModeName
            << " resolved="
            << summary.resolved
            << std::endl;

        std::cout << "      requested="
            << summary.requestedValueText
            << std::endl;

        std::cout << "      resolvedArcLength="
            << summary.resolvedArcLength
            << std::endl;

        debugPrintVec3("P", summary.entryFrame.P);
        debugPrintVec3("T", summary.entryFrame.T);
        debugPrintVec3("N", summary.entryFrame.N);
        debugPrintVec3("B", summary.entryFrame.B);
    }

}