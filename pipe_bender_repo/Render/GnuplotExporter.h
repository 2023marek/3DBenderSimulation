#pragma once

#include <fstream>
#include <iostream>

#include "../Core/PipeAxis2D.h"
#include "../Machine/MachineModel.h"
#include "../Core/Math/Vec2D.h"

class GnuplotExporter
{
public:
    static void exportPipe(const PipeAxis2D& pipe,
        const std::string& filename = "pipe.dat");

    static void exportPipe(const PipeAxis2D& pipe,
        const std::string& filename)
    {
        std::ofstream file(filename);
        writePipe(file, pipe);
    }
     static void exportMachine(const MachineModel& machine,
        const std::string& filename = "machine.dat");
     static void exportMachine(const MachineModel& machine,
         const std::string& filename)
     {
         std::ofstream file(filename);
         writeMachine(file, machine);
     }



    static void exportScene(const PipeAxis2D& pipe,
        const MachineModel& machine);

    static void exportScene(const PipeAxis2D& pipe,
        const MachineModel& machine);

    void exportScene(const PipeAxis2D& pipe,
        const MachineModel& machine)
    {
        exportPipe(pipe, "pipe.dat");
        exportMachine(machine, "machine.dat");
    }
    

private:
    // --- Internal helpers (implementation details) ---

    static void writePipe(std::ofstream& file,
        const PipeAxis2D& pipe);
    static void writePipe(std::ofstream& file,
        const PipeAxis2D& pipe)
    {
        for (const auto& n : pipe.getNodes())
        {
            file << n.pos.x << " "
                << n.pos.y << "\n";
        }
    }

    static void writeMachine(std::ofstream& file,
        const MachineModel& machine);

    static void writeMachine(std::ofstream& file,
        const MachineModel& machine)
    {
        Vec2 C = machine.bendCenter;
        file << C.x << " " << C.y << "\n";
    }
};
