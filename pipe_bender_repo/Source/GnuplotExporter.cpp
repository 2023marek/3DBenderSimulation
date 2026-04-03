
#include <fstream>
#include "../Core/PipeAxis2D.h"
#include "../Machine/MachineModel.h"
#include "../Core/Math/Vec2D.h" // Ensure this include is before any use of Vec2
#include "../Render/GnuplotExporter.h"
#include <filesystem>
#include <string>

// ================= PUBLIC =================
void runGnuplot(const std::string& scriptFile);  

void GnuplotExporter::exportPipe(const PipeAxis2D& pipe,
    const std::string& filename)

{
    std::ofstream file(filename);
    if (!file)
    {
        std::cerr << "Cannot open " << filename << "\n";
        return;
    }
    file << "# x y theta\n";
    std::cout <<"  x   y   theta\n";
    if (pipe.getNodes().empty())
    {
        std::cerr << "ERROR: Pipe has no nodes!\n";
        return;
    }
    for (const auto& n : pipe.getNodes())
    {
        file << n.pos.x << " "
            << n.pos.y << " "
			<< n.theta << "\n";
        
    }
    file.close();
	std::cout << "Exported " << pipe.getNodes().size() << " nodes to " << filename << "\n";

    std::ofstream script("plot_pipe.plt");
    auto path = std::filesystem::current_path().string();
    std::replace(path.begin(), path.end(), '\\', '/');

    auto fullFile = std::filesystem::absolute(filename).string();
    std::replace(fullFile.begin(), fullFile.end(), '\\', '/');

     

    script << "cd \"" << path << "\"\n";
    script << "set terminal wxt\n";
    script << "set grid\n";
    script << "set size ratio -1\n";
    script << "plot \\\n";
    script << "\"" << fullFile << "\" using 1:2 with lines lw 2 title \"Pipe\", \\\n";
    script << "\"" << fullFile << "\" using 1:2:(75*cos($3)):(75*sin($3)) "
        << "with vectors head filled lt 2 title \"Direction\"\n";
    script << "pause -1\n";
    script.close();
	runGnuplot("plot_pipe.plt");
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
void runGnuplot(const std::string& scriptFile)
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