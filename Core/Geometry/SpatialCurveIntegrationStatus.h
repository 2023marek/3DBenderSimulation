#pragma once

enum class SpatialCurveIntegrationStatus
{
    NotStarted,

    Completed,

    InvalidStartFrame,

    InvalidProfile,

    InvalidSampleStep,

    EmptyProfile,

    StepLimitReached,

    NumericalFailure
};