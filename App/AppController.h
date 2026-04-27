#pragma once

#include "Core/SimulationController.h"
#include "Render/HUDData.h"   

class AppController
{
public:
    AppController();

    void update(double dt);

    const PipeAxis3D& getPipeGeometry() const;

    // NEW
    HUDData buildHUDData() const;

private:
    SimulationController sim;
};