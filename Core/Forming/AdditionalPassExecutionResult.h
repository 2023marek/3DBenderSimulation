#pragma once

enum class AdditionalPassExecutionResult
{
    Validated,
    Executed,
    Disabled,
    UnsupportedProcess,
    InvalidEntryFrame
};

const char* additionalPassExecutionResultToString(
    AdditionalPassExecutionResult result
);

