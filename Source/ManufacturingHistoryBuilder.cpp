
#include <sstream>
#include "Core/Forming/ManufacturingHistoryBuilder.h"

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
            << ": ";

        switch (type)
        {
        case TubeFormingProcessType::RotaryDrawBending:
            out << "rotary draw forming pass";
            break;

        case TubeFormingProcessType::HelixForming:
            out << "helix forming pass";
            break;

        case TubeFormingProcessType::TwoRollerContinuous:
            out << "two-roller forming pass";
            break;

        case TubeFormingProcessType::StretchBending:
            out << "stretch-bending forming pass";
            break;

        default:
            out << "forming pass";
            break;
        }

        return out.str();
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

        extra.entryFrame.P = { 0.0, 0.0, 0.0 };
        extra.entryFrame.T = { 1.0, 0.0, 0.0 };
        extra.entryFrame.N = { 0.0, 1.0, 0.0 };
        extra.entryFrame.B = { 0.0, 0.0, 1.0 };

        history.additionalPasses.push_back(
            extra
        );
    }
}