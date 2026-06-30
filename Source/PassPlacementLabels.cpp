#include "Core/Forming/PassPlacementLabels.h"

const char* passPlacementModeToLabel(
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