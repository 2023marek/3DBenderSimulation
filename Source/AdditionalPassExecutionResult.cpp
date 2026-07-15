#include "Core/Forming/AdditionalPassExecutionResult.h"

const char* additionalPassExecutionResultToString(
    AdditionalPassExecutionResult result
)
{
    switch (result)
    {
    case AdditionalPassExecutionResult::Validated:
        return "Validated";

    case AdditionalPassExecutionResult::Executed:
        return "Executed";

    case AdditionalPassExecutionResult::Disabled:
        return "Disabled";

    case AdditionalPassExecutionResult::UnsupportedProcess:
        return "UnsupportedProcess";

    case  AdditionalPassExecutionResult::InvalidEntryFrame:
        return "InvalidEntryFrame";

    default:
        return "Unknown";
    }
}