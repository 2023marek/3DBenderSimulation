#pragma once

#include <iostream>
#include <cmath>
#include "Core/BendDirection.h"
#include "Core/RotationDirection.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif



// =========================================================================
// OPERATION STRUCTURE - User-facing API for pipe operations
// =========================================================================
//
// This structure represents a single operation that the pipe bender
// can perform. Three types are supported:
//
//   FEED:   Move forward in a straight line
//   BEND:   Curve with a given radius and angle
//   ROTATE: Twist around the longitudinal axis
//
// =========================================================================

struct Operation
{
    enum Type
    {
        FEED,
        BEND,
        ROTATE
    };

    Type type = FEED;

    // FEED
    double length = 0.0;

    // BEND
    double R = 0.0;
    double angle = 0.0;

    // Angle is positive; direction controls sign.
    BendDirection bendDirection = BendDirection::CCW;
    RotationDirection rotationDirection = RotationDirection::CCW;
    // Progress
    double progress = 0.0;

    void print() const
    {
        if (type == FEED)
        {
            std::cout << "FEED " << length << " mm";
        }
        else if (type == BEND)
        {
            double angleDegrees = angle * 180.0 / PI;

            std::cout << "BEND R="
                << R
                << " mm, angle="
                << angleDegrees
                << " deg ("
                << bendDirectionToString(bendDirection)
                << ")";
        }
        else if (type == ROTATE)
        {
            double angleDegrees = angle * 180.0 / PI;

            std::cout << "ROTATE "
                << angleDegrees
                << " deg";
        }
        else
        {
            std::cout << "UNKNOWN";
        }

        if (progress > 0.0 && progress < 1.0)
        {
            std::cout << " ["
                << (progress * 100.0)
                << "%]";
        }

        std::cout << "\n";
    }
};