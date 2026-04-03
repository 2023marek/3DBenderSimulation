#pragma once

#include <fstream>
#include <iostream>

#include "../Core/PipeAxis2D.h"
#include "../Machine/MachineModel.h"
#include "../Core/Math/Vec2D.h"

class GnuplotExporter
{
public:
    // Simple API
    static void exportPipe(const PipeAxis2D& pipe,
        const std::string& filename = "pipe.dat");

    static void exportMachine(const MachineModel& machine,
        const std::string& filename = "machine.dat");

    static void exportScene(const PipeAxis2D& pipe,
        const MachineModel& machine);
}; 