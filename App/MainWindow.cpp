#include "MainWindow.h"
#include "Core/SimulationController.h"

MainWindow::MainWindow()
{
    // tworzymy widget renderuj¹cy
    view = new GLView();
    setCentralWidget(view);

    // ?? pod³¹czamy dane (WA¯NE)
    view->setPipe(&controller.getPipeGeometry());

    // timer ~60 FPS
    connect(&timer, &QTimer::timeout, this, &MainWindow::onUpdate);
    timer.start(16);
}

void MainWindow::onUpdate()
{
    // 1. update symulacji
    controller.update(0.016);

    // 2. odœwie¿ render
    view->update();
}