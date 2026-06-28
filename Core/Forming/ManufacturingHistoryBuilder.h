#pragma once

#include "Core/Forming/ManufacturingHistory.h"
#include "Core/Forming/ManufacturingPlan.h"

void buildManufacturingHistoryFromPlan(
    const ManufacturingPlan& plan,
    ManufacturingHistory& history
);