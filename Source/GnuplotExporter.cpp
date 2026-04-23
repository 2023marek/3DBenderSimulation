 
#include <fstream>
#include "../Core/PipeAxis2D.h"
#include "../Machine/MachineModel.h"
#include "../Core/Math/Vec2D.h" // Ensure this include is before any use of Vec2
#include "../Render/GnuplotExporter.h"
#include <filesystem>
#include <string>

// ================= PUBLIC =================
void GnuplotExporter::exportPipe(const PipeAxis2D& pipe)
{
    const std::string dataFile = "pipe.dat";
    const std::string scriptFile = "plot_pipe.plt";

    exportPipeData(pipe, dataFile);
    createPlotScript(dataFile, scriptFile);
    runGnuplot(scriptFile);
}

void GnuplotExporter::exportPipeData(const PipeAxis2D& pipe,
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
            << n.theta << "\n";
    }

    file.close();
}

    void GnuplotExporter::createPlotScript(const std::string & dataFile,
        const std::string & scriptFile)
    {
        std::ofstream script(scriptFile);

        script << "set terminal wxt\n";
        script << "set grid\n";
        script << "set size ratio -1\n";

        script << "while (1) {\n";
        script << "plot \\\n";
        script << "\"" << dataFile << "\" using 1:2 with lines lw 2 title 'Pipe', \\\n";
        script << "\"" << dataFile << "\" every 5 using 1:2:(5*cos($3)):(5*sin($3)) "
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

void GnuplotExporter::exportScene(const PipeAxis2D& pipe,
    const MachineModel& machine)
{
    exportPipe(pipe);
    exportMachine(machine);
}
