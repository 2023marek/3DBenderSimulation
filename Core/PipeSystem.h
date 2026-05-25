#pragma once

#include "Core/PipeAxis3D.h"

// =====================================================
// PIPESYSTEM
//
// PHASE 3 BRIDGE WRAPPER
//
// Current purpose:
//     Own the existing PipeAxis3D implementation.
//
// Future purpose:
//     Own separate pipe subsystems:
//
//         GeometricPipeModel
//         ManufacturingPipeSimulator
//         PhysicalPipeSimulator
//
// Important:
//     For now, this class changes architecture only.
//     It should not change simulation behavior.
// =====================================================

class PipeSystem
{
public:
    PipeSystem()
        : pipeAxis(0.5)
    {
    }

    explicit PipeSystem(double sampleStep)
        : pipeAxis(sampleStep)
    {
    }

    PipeAxis3D& manufacturingPipe()
    {
        return pipeAxis;
    }

    const PipeAxis3D& manufacturingPipe() const
    {
        return pipeAxis;
    }

    PipeAxis3D& legacyPipeAxis()
    {
        return pipeAxis;
    }

    const PipeAxis3D& legacyPipeAxis() const
    {
        return pipeAxis;
    }

    void reset()
    {
        pipeAxis.clear();
    }

private:
    PipeAxis3D pipeAxis;
};