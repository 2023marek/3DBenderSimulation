#include "Core/Forming/ManufacturingHistoryDebug.h"

#include <iostream>



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
    }
}