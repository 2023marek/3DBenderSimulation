#pragma once
#include "Core/SimulationController.h"

// "Mózg" aplikacji – ³¹czy UI z symulacj¹
class AppController
{
public:
    AppController();
    const PipeAxis3D& getPipeGeometry() const;
    // update symulacji (wywo³ywany co frame)
    void update(double dt);

   
private:
    SimulationController sim;
    
};