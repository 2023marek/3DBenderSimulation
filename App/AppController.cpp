#include "AppController.h"


AppController::AppController()
{
    // TODO: init programu (np. load operacji)
}

void AppController::update(double dt)
{
    sim.update(dt);  // delegujemy do Core
}

const PipeAxis3D& AppController::getPipeGeometry() const
{
    return sim.getPipeGeometry();
}