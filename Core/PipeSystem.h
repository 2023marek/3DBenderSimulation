#pragma once

#include "Core/PipeAxis3D.h"
#include "Core/Geometry/GeometricPipeModel.h"
#include "Core/Manufacturing/ManufacturingPipeSimulator.h"

// =====================================================
// PIPESYSTEM
//
// Owns pipe-related subsystems.
//
// Phase 4 status:
// - GeometricPipeModel = CAD preview / ideal geometry
// - ManufacturingPipeSimulator = manufacturing playback
//
// ManufacturingPipeSimulator still wraps old PipeAxis3D.
// Later PipeAxis3D manufacturing state will move into it.
// =====================================================

class PipeSystem
{
public:
    PipeSystem()
        : cadModel(0.5),
        manufacturing(0.5)
    {
    }

    explicit PipeSystem(double sampleStep)
        : cadModel(sampleStep),
        manufacturing(sampleStep)
    {
    }

    // -----------------------------------------------------
    // CAD / ideal geometric pipe
    // -----------------------------------------------------

    GeometricPipeModel& cadPipe()
    {
        return cadModel;
    }

    const GeometricPipeModel& cadPipe() const
    {
        return cadModel;
    }

    // -----------------------------------------------------
    // Manufacturing simulator
    // -----------------------------------------------------

    ManufacturingPipeSimulator& manufacturingPipe()
    {
        return manufacturing;
    }

    const ManufacturingPipeSimulator& manufacturingPipe() const
    {
        return manufacturing;
    }

    // -----------------------------------------------------
    // Temporary legacy access
    //
    // Existing render/app code may still need PipeAxis3D.
    // -----------------------------------------------------

    


    void reset()
    {
        cadModel.clear();
        manufacturing.reset();
    }

    void setProgram(const std::vector<Operation>& operations)
    {
        cadModel.setOperations(operations);
    }

private:
    GeometricPipeModel cadModel;
    ManufacturingPipeSimulator manufacturing;
};