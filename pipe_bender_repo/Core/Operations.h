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
    // =====================================================================
    // OPERATION TYPE ENUMERATION
    // =====================================================================
    //
    // Defines what action this operation performs:
    //
    //   FEED    - Linear motion forward
    //   BEND    - Arc motion (with radius and angle)
    //   ROTATE  - Twist around pipe axis (NEW!)
    //
    enum Type
    {
        FEED,      // ? Move straight
        BEND,      // ? Curve in 3D space
        ROTATE     // ? Twist (NEW)
    };

    // =====================================================================
    // OPERATION PARAMETERS
    // =====================================================================

    Type type = FEED;              // What operation to perform

    // --- For FEED ---
    double length = 0.0;           // mm to move (negative = retract)

    // --- For BEND ---
    double R = 0.0;                // Radius of curve (mm)
    double angle = 0.0;            // Angle of bend (radians)
    BendDirection dir = BendDirection::CCW;  // Curve direction

    // --- For ROTATE ---
    // Uses 'angle' field: rotation amount (radians)

    // --- Progress tracking ---
    double progress = 0.0;          // 0.0 to 1.0 (how far into operation)

    // =====================================================================
    // DEBUG OUTPUT - Show what this operation does
    // =====================================================================
    //
    // Called to print operation details to console
    // Useful for debugging and understanding program flow
    //
    void print() const
    {
        if (type == FEED)
        {
            // FEED: Show distance to move
            std::cout << "FEED " << length << " mm";
        }
        else if (type == BEND)
        {
            // BEND: Show radius, angle, and direction
            double angleDegrees = angle * 180.0 / PI;
            std::cout << "BEND R=" << R << " mm, angle=" << angleDegrees << "°";
            std::cout << " (" << (dir == BendDirection::CCW ? "CCW" : "CW") << ")";
        }
        else if (type == ROTATE)
        {
            // ROTATE: Show twist angle
            double angleDegrees = angle * 180.0 / PI;
            std::cout << "ROTATE " << angleDegrees << "°";
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