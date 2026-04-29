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
public:
    void play() { sim.play(); }
    void pause() { sim.pause(); }
    void reset() { sim.reset(); }
    void step() { sim.step(); }

private:
    SimulationController sim;
};