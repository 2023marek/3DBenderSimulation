#pragma once

enum class AdditionalPassExecutionResult
{
    Validated,
    Executed,
    Disabled,
    UnsupportedProcess,
    InvalidEntryFrame,
    InvalidDeformableRegion

};

const char* additionalPassExecutionResultToString(
    AdditionalPassExecutionResult result
);

