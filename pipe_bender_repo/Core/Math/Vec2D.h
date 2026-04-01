#pragma once

#include <cmath>

struct Vec2
{
    double x;
    double y;

    Vec2()
        : x(0.0), y(0.0)
    {
    }

    Vec2(double X, double Y)
        : x(X), y(Y)
    {
    }
};
