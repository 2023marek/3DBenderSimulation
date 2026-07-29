#include "Core/Forming/StretchBendingManufacturingStage.h"

const char*
stretchBendingManufacturingStageToString(
    StretchBendingManufacturingStage stage
)
{
    switch (stage)
    {
    case StretchBendingManufacturingStage::Invalid:
        return "Invalid";

    case StretchBendingManufacturingStage::Ready:
        return "Ready";

    case StretchBendingManufacturingStage::ApplyingTension:
        return "ApplyingTension";

    case StretchBendingManufacturingStage::Forming:
        return "Forming";

    case StretchBendingManufacturingStage::LoadedHold:
        return "LoadedHold";

    case StretchBendingManufacturingStage::Unloading:
        return "Unloading";

    case StretchBendingManufacturingStage::Complete:
        return "Complete";
    }

    return "Unknown";
}