#include "MainWindow.h"
#include "Core/SimulationController.h"

MainWindow::MainWindow()
{
    // create view
    view = new GLView();
    setCentralWidget(view);

    // ? CONNECT CONTROLLER ? VIEW
    view->setAppController(&controller);   // ? ADD THIS LINE

    // ? CONNECT PIPE DATA
    view->setPipe(&controller.getPipeGeometry());

    // timer ~60 FPS
    connect(&timer, &QTimer::timeout, this, &MainWindow::onUpdate);
    timer.start(16);
}
void MainWindow::onUpdate()
{
    // 1. update symulacji
    controller.update(0.016);

    // 2.  render
    view->update();
}