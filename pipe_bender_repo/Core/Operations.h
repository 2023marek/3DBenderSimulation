#pragma once
#include <iostream>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846
#endif

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
    double length = 0.0;        // mm (negative = retract)
    double R = 0.0;             // radius (mm)
    double angle = 0.0;         // radians
    BendDirection dir = BendDirection::CCW;
    double progress = 0.0;       // 0.0 to 1.0

    /// Debug output - IMPLEMENTATION (not just declaration!)
    void print() const
    {
        if (type == FEED)
        {
            std::cout << "FEED " << length << " mm";
        }
        else if (type == BEND)
        {
            double angleDegrees = angle * 180.0 / PI;
            std::cout << "BEND R=" << R << " mm, angle=" << angleDegrees << "°";
            std::cout << " (" << (dir == BendDirection::CCW ? "CCW" : "CW") << ")";
        }
        else
        {
            std::cout << "UNKNOWN";
        }

        // Show progress if operation is in progress
        if (progress > 0.0 && progress < 1.0)
            std::cout << " [" << (progress * 100.0) << "%]";

        std::cout << "\n";
    }
};