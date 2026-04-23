
#include <fstream>
#include "../Core/PipeAxis3D.h"
#include "../Machine/MachineModel.h"
#include "../Core/Math/Vec3D.h" // Ensure this include is before any use of Vec2
#include "../Render/GnuplotExporter3D.h"
#include <filesystem>
#include <string>

// ================= PUBLIC =================
void GnuplotExporter::exportPipe3D(const PipeAxis3D& pipe)
{
    const std::string dataFile = "pipe3d.dat";
    const std::string scriptFile = "plot_pipe3d.plt";

    exportPipeData3D(pipe, dataFile);
    createPlotScript3D(dataFile, scriptFile);
    runGnuplot(scriptFile);
}
void GnuplotExporter::exportPipeData3D(const PipeAxis3D& pipe,
    const std::string& filename)
{
    std::ofstream file(filename);

    if (!file)
    {
        std::cerr << "Cannot open " << filename << "\n";
        return;
    }

    for (const auto& n : pipe.getNodes())
    {
        file << n.pos.x << " "
            << n.pos.y << " "
            << n.pos.z << " "
            << n.T.x << " "
            << n.T.y << " "
            << n.T.z << "\n";
    }

    file.close();
}
void GnuplotExporter::createPlotScript3D(const std::string& dataFile,
    const std::string& scriptFile)
{
    std::ofstream script(scriptFile);

    script << "set terminal wxt\n";
    script << "set grid\n";
    script << "set view equal xyz\n";

    script << "while (1) {\n";

    script << "splot \\\n";

    // pipe line
    script << "\"" << dataFile << "\" using 1:2:3 with lines lw 2 title 'Pipe', \\\n";

    // direction vectors (scaled)
    script << "\"" << dataFile
        << "\" every 5 using 1:2:3:(5*$4):(5*$5):(5*$6) "
        << "with vectors head filled title 'Direction'\n";

    script << "pause 1\n";
    script << "}\n";

    script.close();
}

void GnuplotExporter::runGnuplot(const std::string& scriptFile)
{
    std::string cmd = "gnuplot " + scriptFile;

    int result = system(cmd.c_str());

    if (result != 0)
    {
        cmd = "\"C:\\Program Files\\gnuplot\\bin\\gnuplot.exe\" " + scriptFile;
        result = system(cmd.c_str());
    }

    if (result != 0)
    {
        std::cerr << "Failed to launch gnuplot\n";
    }
}








void GnuplotExporter::exportMachine(const MachineModel& machine,
    const std::string& filename)
{
    std::ofstream file(filename);

    Vec2D C = machine.bendCenter;
    file << C.x << " " << C.y << "\n";
}

