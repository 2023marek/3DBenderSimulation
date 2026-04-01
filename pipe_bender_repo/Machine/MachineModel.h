#pragma once

#include "../core/Vec2.h"

struct MachineModel
{
    Vec2 bendCenter{ 0,0 };

    double bendRadius = 50.0;

    double clampLength = 80.0;

    double entryLength = 200.0;

};