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
            return "Append";

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

        std::cout << "      placementMode="
            << placementModeToString(extra.resolvedPlacementMode)
            << " resolved="
            << extra.hasResolvedPlacement
            << std::endl;

        std::cout << "      nodeIndex="
            << extra.resolvedNodeIndex
            << " requestedArcLength="
            << extra.requestedArcLength
            << " resolvedArcLength="
            << extra.resolvedArcLength
            << std::endl;
        debugPrintVec3("P", extra.entryFrame.P);
        debugPrintVec3("T", extra.entryFrame.T);
        debugPrintVec3("N", extra.entryFrame.N);
        debugPrintVec3("B", extra.entryFrame.B);


    }


}