#pragma once

#include <cmath>

struct Vec2D
{
    double x;
    double y;

    Vec2D()
        : x(0.0), y(0.0)
    {
    }

    Vec2D(double X, double Y)
        : x(X), y(Y)
    {
    }
};
