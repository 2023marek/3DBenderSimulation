#include <QPushButton>
#include "Common/UserAction.h"
#include "MainWindow.h"
#include "Core/SimulationController.h"

MainWindow::MainWindow()
{
    // =========================
    // VIEW
    // =========================
    view = new GLView();
    setCentralWidget(view);
    view->setFocus();
    view->setAppController(&controller);
    view->setPipe(&controller.getPipeGeometry());
    view->setFocus(); // important for keyboard

    // =========================
    // BUTTONS
    // =========================
    QPushButton* btnPlay = new QPushButton("Play", this);
    btnPlay->setGeometry(10, 220, 60, 30);

    QPushButton* btnPause = new QPushButton("Pause", this);
    btnPause->setGeometry(80, 220, 60, 30);

    QPushButton* btnReset = new QPushButton("Reset", this);
    btnReset->setGeometry(150, 220, 60, 30);

    // =========================
    // CONNECT SIGNALS ? CONTROLLER
    // =========================
    connect(btnPlay, &QPushButton::clicked, this, [&]() {
        controller.handleAction(UserAction::Play);
        });

    connect(btnPause, &QPushButton::clicked, this, [&]() {
        controller.handleAction(UserAction::Pause);
        });

    connect(btnReset, &QPushButton::clicked, this, [&]() {
        controller.handleAction(UserAction::Reset);
        });

    // =========================
    // TIMER
    // =========================
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
