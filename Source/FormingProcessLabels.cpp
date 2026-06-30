#include "Core/Forming/FormingProcessLabels.h"

const char* formingProcessTypeToLabel(
    TubeFormingProcessType type
)
{
    switch (type)
    {
    case TubeFormingProcessType::RotaryDrawBending:
        return "RotaryDraw";

    case TubeFormingProcessType::ManualRework:
        return "ManualRework";

    case TubeFormingProcessType::HelixForming:
        return "HelixForming";

    case TubeFormingProcessType::StretchBending:
        return "StretchBending";

    case TubeFormingProcessType::RollerForming:
        return "RollerForming";

    case TubeFormingProcessType::TwoRollerContinuous:
        return "TwoRollerContinuous";

    default:
        return "Unknown";
    }
}