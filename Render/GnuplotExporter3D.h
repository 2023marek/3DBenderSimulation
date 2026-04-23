#pragma once
#include <fstream>
#include <iostream>

#include "../Core/PipeAxis3D.h"
#include "../Machine/MachineModel.h"
#include "../Core/Math/Vec3D.h"

class GnuplotExporter
{


public:
    static void exportPipe3D(const PipeAxis3D& pipe);

private:
    static void exportPipeData3D(const PipeAxis3D& pipe, const std::string& file);
    static void createPlotScript3D(const std::string& dataFile, const std::string& scriptFile);
    static void runGnuplot(const std::string& scriptFile);

public:
    static void exportMachine(const MachineModel& machine,
        const std::string& filename = "machine.dat");

  


};