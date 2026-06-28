#include "Core/Forming/ManufacturingHistoryBuilder.h"
namespace
{
    const char* additionalPassNameFromProcessType(
        TubeFormingProcessType type
    )
    {
        switch (type)
        {
        case TubeFormingProcessType::RotaryDrawBending:
            return "Additional rotary draw forming pass";

        case TubeFormingProcessType::HelixForming:
            return "Additional helix forming pass";

        case TubeFormingProcessType::TwoRollerContinuous:
            return "Additional two-roller forming pass";

        case TubeFormingProcessType::StretchBending:
            return "Additional stretch-bending forming pass";

        default:
            return "Additional forming pass";
        }
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

        extra.name =
            additionalPassNameFromProcessType(
                pass.processType
            );

        extra.pass =
            pass;

        extra.entryFrame.P = { 0.0, 0.0, 0.0 };
        extra.entryFrame.T = { 1.0, 0.0, 0.0 };
        extra.entryFrame.N = { 0.0, 1.0, 0.0 };
        extra.entryFrame.B = { 0.0, 0.0, 1.0 };

        history.additionalPasses.push_back(
            extra
        );
    }
}