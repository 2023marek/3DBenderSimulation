#pragma once

#include <vector>
#include <cmath>

#include "Core/Machine/MachineRenderData.h"
#include "Core/Math/Vec3D.h"
#include "Core/Geometry/Frame.h"

// =====================================================
// MACHINE REFERENCE RENDERER
//
// Builds simple machine reference line strips.
//
// This class does NOT own simulation state.
// This class does NOT modify machine or pipe geometry.
// It only converts MachineRenderData into drawable line strips.
// =====================================================

class MachineReferenceRenderer
{
public:
    static std::vector<std::vector<float>> buildLineStrips(
        const MachineRenderData& data)
    {
        std::vector<std::vector<float>> strips;

        const Frame& frame =
            data.machineEntryFrame;

        Vec3D P = frame.P;

        Vec3D T = frame.T.normalized();
        Vec3D N = frame.N.normalized();
        Vec3D B = frame.B.normalized();

        if (T.lengthSquared() < 1e-12)
            T = { 1.0, 0.0, 0.0 };

        if (N.lengthSquared() < 1e-12)
            N = { 0.0, 1.0, 0.0 };

        if (B.lengthSquared() < 1e-12)
            B = { 0.0, 0.0, 1.0 };

        double axisLength =
            60.0;

        double dieRadius =
            data.bendDieRadius > 1e-9
            ? data.bendDieRadius
            : 20.0;

        Vec3D bendDieCenter =
            data.bendDieCenter;

        // Entry tangent axis
        strips.push_back(
            pointsToFloatLine({
                P,
                P + T * axisLength
                })
        );

        // Normal axis
        strips.push_back(
            pointsToFloatLine({
                P,
                P + N * axisLength
                })
        );

        // Binormal / bend axis
        strips.push_back(
            pointsToFloatLine({
                P,
                P + B * axisLength
                })
        );

        // Bend die circle in T-N plane
        std::vector<Vec3D> circle;

        constexpr double kPi =
            3.14159265358979323846;

        const int circleSegments =
            72;

        for (int i = 0; i <= circleSegments; ++i)
        {
            double a =
                2.0 * kPi * static_cast<double>(i)
                / static_cast<double>(circleSegments);

            Vec3D point =
                bendDieCenter
                + T * (std::cos(a) * dieRadius)
                + N * (std::sin(a) * dieRadius);

            circle.push_back(point);
        }

        strips.push_back(
            pointsToFloatLine(circle)
        );

        // Bend direction reference line
        double bendSign =
            bendDirectionSign(data.bendDirection);

        strips.push_back(
            pointsToFloatLine({
                bendDieCenter,
                bendDieCenter + N * (dieRadius * bendSign)
                })
        );

        return strips;
    }

private:
    static std::vector<float> pointsToFloatLine(
        const std::vector<Vec3D>& points)
    {
        std::vector<float> data;
        data.reserve(points.size() * 3);

        for (const auto& p : points)
        {
            data.push_back(static_cast<float>(p.x));
            data.push_back(static_cast<float>(p.y));
            data.push_back(static_cast<float>(p.z));
        }

        return data;
    }
};