#pragma once

#include "../core/Math/Vec2D.h"

struct MachineModel
{
    Vec2D bendCenter{ 0,0 };

    double bendRadius = 50.0;

    double clampLength = 80.0;

    double entryLength = 200.0;

};