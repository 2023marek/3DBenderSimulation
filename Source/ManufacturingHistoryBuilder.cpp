#include "Core/Forming/ManufacturingHistoryBuilder.h"

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

    if (plan.passes.size() > 1)
    {
        AdditionalFormingPass extra;

        extra.name =
            "Additional helix forming pass";

        extra.pass =
            plan.passes[1];

        extra.entryFrame.P = { 0.0, 0.0, 0.0 };
        extra.entryFrame.T = { 1.0, 0.0, 0.0 };
        extra.entryFrame.N = { 0.0, 1.0, 0.0 };
        extra.entryFrame.B = { 0.0, 0.0, 1.0 };

        history.additionalPasses.push_back(
            extra
        );
    }
}