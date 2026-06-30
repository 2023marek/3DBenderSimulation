#include "Core/Forming/ManufacturingHistoryDebug.h"
#include "Core/Math/Vec3D.h"
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

    std::cout << "  additional passes: "
        << history.additionalPasses.size()
        << std::endl;

    for (size_t i = 0; i < history.additionalPasses.size(); ++i)
    {
        const AdditionalFormingPass& extra =
            history.additionalPasses[i];

        std::cout << "    [" << i << "] name="
            << extra.name
            << " process="
            << manufacturingProcessTypeToString(extra.pass.processType)
            << std::endl;

        debugPrintPlacementDetails(
            extra
        );
        debugPrintVec3("P", extra.entryFrame.P);
        debugPrintVec3("T", extra.entryFrame.T);
        debugPrintVec3("N", extra.entryFrame.N);
        debugPrintVec3("B", extra.entryFrame.B);


    }


}