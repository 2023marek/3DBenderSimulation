#pragma once

enum class BendDirection
{
    CCW = 1,
    CW = -1
};

struct Operation
{
    enum Type
    {
        FEED,
        BEND
    };

    Type type = FEED;

    double length = 0.0;
    double R = 0.0;
    double angle = 0.0;

    BendDirection dir = BendDirection::CCW;

    double progress = 0.0;
};
