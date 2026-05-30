#pragma once

#include "Core/Geometry/GeometricPipeModel.h"
#include "Core/Manufacturing/ManufacturingPipeSimulator.h"

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

    GeometricPipeModel& cadPipe()
    {
        return cadModel;
    }

    const GeometricPipeModel& cadPipe() const
    {
        return cadModel;
    }

    ManufacturingPipeSimulator& manufacturingPipe()
    {
        return manufacturing;
    }

    const ManufacturingPipeSimulator& manufacturingPipe() const
    {
        return manufacturing;
    }

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