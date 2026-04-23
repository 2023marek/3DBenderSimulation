#pragma once

#include <fstream>
#include <iostream>

#include "../Core/PipeAxis2D.h"
#include "../Machine/MachineModel.h"
#include "../Core/Math/Vec2D.h"

class GnuplotExporter
{


public:
    static void exportPipe(const PipeAxis2D& pipe);

private:
    static void exportPipeData(const PipeAxis2D& pipe, const std::string& file);
    static void createPlotScript(const std::string& dataFile, const std::string& scriptFile);
    static void runGnuplot(const std::string& scriptFile);

public:
    static void exportMachine(const MachineModel& machine,
        const std::string& filename = "machine.dat");

    static void exportScene(const PipeAxis2D& pipe,
        const MachineModel& machine);



}; 