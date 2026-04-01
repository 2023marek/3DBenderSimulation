
#include <fstream>
#include "../Core/PipeAxis2D.h"
#include "../Machine/MachineModel.h"
#include "../Core/Math/Vec2D.h" // Ensure this include is before any use of Vec2
#include "../Render/GnuplotExporter.h"
// ================= PUBLIC =================

void GnuplotExporter::exportPipe(const PipeAxis2D& pipe,
    const std::string& filename)
{
    std::ofstream file(filename);

    for (const auto& n : pipe.getNodes())
    {
        file << n.pos.x << " "
            << n.pos.y << "\n";
    }
}

void GnuplotExporter::exportMachine(const MachineModel& machine,
    const std::string& filename)
{
    std::ofstream file(filename);

    Vec2D C = machine.bendCenter;
    file << C.x << " " << C.y << "\n";
}

void GnuplotExporter::exportScene(const PipeAxis2D& pipe,
    const MachineModel& machine)
{
    exportPipe(pipe);
    exportMachine(machine);
}