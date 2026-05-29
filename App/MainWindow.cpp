#include <QPushButton>
#include <QKeyEvent>
#include "Common/UserAction.h"
#include "MainWindow.h"
#include "Core/SimulationController.h"
#include "Render/RenderMode.h"

MainWindow::MainWindow()
{
    // =========================
    // VIEW
    // =========================
    view = new GLView();
    
    setCentralWidget(view);
    view->setFocus();
    view->setAppController(&controller);
    view->setAppController(&controller);
    view->setFocus(); // important for keyboard

    // =========================
    // BUTTONS
    // =========================
    QPushButton* btnPlay = new QPushButton("Play", this);
    btnPlay->setGeometry(10, 220, 60, 30);
    btnPlay->setFocusPolicy(Qt::NoFocus);   // ? HERE

    QPushButton* btnPause = new QPushButton("Pause", this);
    btnPause->setGeometry(80, 220, 60, 30);
    btnPause->setFocusPolicy(Qt::NoFocus);  // ? HERE

    QPushButton* btnReset = new QPushButton("Reset", this);
    btnReset->setGeometry(150, 220, 60, 30);
    btnReset->setFocusPolicy(Qt::NoFocus);  // ? HERE

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
    controller.update(0.016);

    HUDData data = controller.buildHUDData();

    // ONLY pass data
    view->setHUDData(data);
    view->setRenderMode(controller.getRenderMode());

    // trigger rendering
    view->update();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    std::cout << "[KEY GLOBAL] key=" << event->key() << std::endl;

    switch (event->key())
    {
    case Qt::Key_Space:
        std::cout << "[KEY] SPACE ? PLAY\n";
        controller.handleAction(UserAction::Play);
        break;

    case Qt::Key_P:
        std::cout << "[KEY] P ? PAUSE\n";
        controller.handleAction(UserAction::Pause);
        break;

    case Qt::Key_R:
        std::cout << "[KEY] R ? RESET\n";
        controller.handleAction(UserAction::Reset);
        break;

    case Qt::Key_S:
        std::cout << "[KEY] S ? STEP\n";
        controller.handleAction(UserAction::Step);
        break;

    case Qt::Key_M:
        std::cout << "[KEY] M ? TOGGLE MODE\n";
        controller.handleAction(UserAction::ToggleRenderMode);

        std::cout << "[MODE SWITCH] "
            << (controller.getRenderMode() == RenderMode::LINE ? "LINE" : "MESH")
            << std::endl;
        break;
    }

    // force redraw
    view->update();
}