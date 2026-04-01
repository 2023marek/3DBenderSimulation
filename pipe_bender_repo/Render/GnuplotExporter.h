#pragma once

#include <fstream>
#include <iostream>

#include "../Core/PipeAxis2D.h"
#include "../Machine/MachineModel.h"

class GnuplotExporter
{
public:

    static void exportScene(const PipeAxis2D& pipe,
        const MachineModel& machine)
    {
        exportPipe(pipe, "pipe.dat");
        exportMachine(machine, "machine.dat");
    }

private:

    static void exportPipe(const PipeAxis2D& pipe,
        const std::string& filename)
    {
        std::ofstream file(filename);

        for (const auto& n : pipe.getNodes())
        {
            file << n.pos.x << " "
                << n.pos.y << "\n";
        }
    }

    static void exportMachine(const MachineModel& machine,
        const std::string& filename)
    {
        std::ofstream file(filename);

        Vec2 C = machine.bendCenter;

        file << C.x << " " << C.y << "\n";
    }
};
