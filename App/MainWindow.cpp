#include <QPushButton>
#include <QKeyEvent>
#include "Common/UserAction.h"
#include "MainWindow.h"
#include "Core/SimulationController.h"
#include "Render/RenderMode.h"
#include <string>

MainWindow::MainWindow()
{
    // =========================
    // VIEW
    // =========================
    view = new GLView();
    
    setCentralWidget(view);
    view->setFocus();
    view->setAppController(&controller);
    view->setFocus(); // important for keyboard
    // =====================================================
// STRETCH-BENDING AUTOMATIC PLAYBACK TIMER
//
// The timer only schedules updates.
//
// Actual process advancement remains owned by:
//     AppController::advanceDebugStretchBendingPlayback()
//
// Geometry rendering remains owned by:
//     GLView::paintGL()
// =====================================================

    stretchPlaybackTimer.setInterval(
        16
    );
    // =========================
    // BUTTONS
    // =========================
    QPushButton* btnPlay = new QPushButton("Play", this);
    btnPlay->setGeometry(10, 320, 60, 30);
    btnPlay->setFocusPolicy(Qt::NoFocus);   // ? HERE

    QPushButton* btnPause = new QPushButton("Pause", this);
    btnPause->setGeometry(80, 320, 60, 30);
    btnPause->setFocusPolicy(Qt::NoFocus);  // ? HERE

    QPushButton* btnReset = new QPushButton("Reset", this);
    btnReset->setGeometry(150, 320, 60, 30);
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
    connect(
        &stretchPlaybackTimer,
        &QTimer::timeout,
        this,
        &MainWindow::updateStretchPlayback
    );

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

    data.stretchPlaying =
        stretchPlaybackRunning;

    data.stretchPlaybackSpeed =
        stretchPlaybackSpeed;
    // later remove it
    data.stretchPreviewVisible =
        view->isStretchPlaybackPreviewVisible();

    data.stretchLoadedPreviewVisible =
        view->isStretchLoadedPreviewVisible();

    data.stretchCurrentPreviewVisible =
        view->isStretchCurrentPreviewVisible();

    data.stretchFinalPreviewVisible =
        view->isStretchFinalPreviewVisible();

    data.stretchActiveZoneMarkersVisible =
        view->areStretchActiveZoneMarkersVisible();
    // ONLY pass data
    view->setHUDData(data);
    view->setRenderMode(controller.getRenderMode());

    // trigger rendering
    view->update();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    std::cout << "[KEY GLOBAL] key="
        << event->key()
        << std::endl;

    switch (event->key())
    {
    case Qt::Key_Space:
        std::cout << "[KEY] SPACE ? PLAY\n";
        controller.handleAction(
            UserAction::Play
        );
        break;

    case Qt::Key_P:
        std::cout << "[KEY] P ? PAUSE\n";
        controller.handleAction(
            UserAction::Pause
        );
        break;

    case Qt::Key_R:
        std::cout << "[KEY] R ? RESET\n";
        controller.handleAction(
            UserAction::Reset
        );
        break;

    case Qt::Key_S:
        std::cout << "[KEY] S ? STEP\n";
        controller.handleAction(
            UserAction::Step
        );
        break;

    case Qt::Key_M:
        std::cout << "[KEY] M ? TOGGLE RENDER MODE\n";
        controller.handleAction(
            UserAction::ToggleRenderMode
        );
        break;

        std::cout << "[RENDER MODE] "
            << (controller.getRenderMode() == RenderMode::LINE
                ? "LINE"
                : "MESH")
            << std::endl;
        break;

    case Qt::Key_N:
        std::cout << "[KEY] N ? TOGGLE SIMULATION MODE\n";
        controller.handleAction(
            UserAction::ToggleSimulationMode
        );
        break;

    case Qt::Key_T:
        std::cout << "[KEY] T ? TOGGLE PLACEMENT PRESET\n";
        controller.handleAction(
            UserAction::TogglePlacementPreset
        );
        break;

    case Qt::Key_D:
        std::cout << "[KEY] D ? TOGGLE PREVIEW DEBUG\n";
        controller.handleAction(
            UserAction::TogglePlannedPreviewDebug
        );
        break;
    case Qt::Key_A:
        std::cout << "[KEY] A ? TOGGLE EXPLICIT ATTACH MODE\n";
        controller.handleAction(
            UserAction::ToggleExplicitAttachMode
        );
        break;

    case Qt::Key_E:
        std::cout << "[KEY] E ? TOGGLE OVERLAY VISIBLE MODE\n";
        controller.handleAction(
            UserAction::ToggleDeformableRegionOverlay
        );
        break;

    case Qt::Key_L:
    {
        std::cout
            << "[KEY] L - TOGGLE STRETCH LOADED PREVIEW\n";

        view->toggleStretchLoadedPreview();

        break;
    }

    case Qt::Key_Z:
    {
        std::cout
            << "[KEY] Z - TOGGLE STRETCH ACTIVE ZONE MARKERS\n";

        view->toggleStretchActiveZoneMarkers();

        break;
    }

    case Qt::Key_O:
    {
        std::cout
            << "[KEY] O - TOGGLE STRETCH PLAYBACK PREVIEW\n";

        view->toggleStretchCurrentPreview();

        break;
    }

    case Qt::Key_F:
    {
        std::cout
            << "[KEY] F - TOGGLE STRETCH FINAL PREVIEW\n";

        view->toggleStretchFinalPreview();

        break;
    }



    case Qt::Key_BracketRight:
    {
        std::cout
            << "[KEY] ] - ADVANCE STRETCH PLAYBACK\n";

        pauseStretchPlayback();

        controller.advanceDebugStretchBendingPlayback(
            0.25
        );

        break;
    }


    case Qt::Key_BracketLeft:
    {
        std::cout
            << "[KEY] [ - RESET STRETCH PLAYBACK\n";

        pauseStretchPlayback();

        controller.resetDebugStretchBendingPlayback();

        break;
    }

    case Qt::Key_B:
    {
        std::cout
            << "[KEY] SPACE - TOGGLE STRETCH PLAYBACK\n";

        toggleStretchPlayback();

        break;
    }

    case Qt::Key_Plus:
    case Qt::Key_Equal:
    {
        std::cout
            << "[KEY] + - INCREASE STRETCH PLAYBACK SPEED\n";

        increaseStretchPlaybackSpeed();

        break;
    }






    case Qt::Key_Minus:
    case Qt::Key_Underscore:
    {
        std::cout
            << "[KEY] - - DECREASE STRETCH PLAYBACK SPEED\n";

        decreaseStretchPlaybackSpeed();

        break;
    }

    case Qt::Key_H:
    {
        std::cout
            << "[KEY] H - ADVANCE STRETCH HELIX WRAP\n";

        controller.advanceDebugStretchHelixWrapping(
            25.0
        );

        break;
    }

    case Qt::Key_J:
    {
        std::cout
            << "[KEY] J - RESET STRETCH HELIX WRAP\n";

        controller.resetDebugStretchHelixWrapping();

        break;
    }
    
    case Qt::Key_1:
    {
        std::cout
            << "[CAMERA] FRONT\n";

        view->setFrontCameraView();

        break;
    }


    case Qt::Key_2:
    {
        std::cout
            << "[CAMERA] LEFT\n";

        view->setLeftCameraView();

        break;
    }


    case Qt::Key_3:
    {
        std::cout
            << "[CAMERA] TOP\n";

        view->setTopCameraView();

        break;
    }


    case Qt::Key_4:
    {
        std::cout
            << "[CAMERA] PERSPECTIVE\n";

        view->setPerspectiveCameraView();

        break;
    }

    case Qt::Key_I:

        controller.handleAction(
            UserAction::
            ToggleSpatialIntegratorPreview
        );
        update();
        break;

    default:
        break;
    }




    view->update();

}

void MainWindow::startStretchPlayback()
{
    if (stretchPlaybackRunning)
        return;
    if (controller.isDebugStretchBendingPlaybackComplete())
    {
        std::cout
            << "[STRETCH PLAYBACK]"
            << " state=NOT_STARTED"
            << " reason=AlreadyComplete"
            << std::endl;

        return;
    }

    // Start measuring from this moment. This prevents a
    // large delta time caused by time spent while paused.
    stretchPlaybackClock.restart();

    stretchPlaybackRunning =
        true;

    stretchPlaybackTimer.start();

    std::cout
        << "[STRETCH PLAYBACK]"
        << " state=PLAYING"
        << " speed="
        << stretchPlaybackSpeed
        << std::endl;
}

void MainWindow::pauseStretchPlayback()
{
    if (!stretchPlaybackRunning)
        return;

    stretchPlaybackTimer.stop();

    stretchPlaybackRunning =
        false;

    std::cout
        << "[STRETCH PLAYBACK]"
        << " state=PAUSED"
        << " speed="
        << stretchPlaybackSpeed
        << std::endl;
}

void MainWindow::toggleStretchPlayback()
{
    if (stretchPlaybackRunning)
    {
        pauseStretchPlayback();
    }
    else
    {
        startStretchPlayback();
    }
}

void MainWindow::updateStretchPlayback()
{
    if (!stretchPlaybackRunning)
        return;

    if (!stretchPlaybackClock.isValid())
    {
        stretchPlaybackClock.restart();
        return;
    }

    const qint64 elapsedMilliseconds =
        stretchPlaybackClock.restart();

    const double rawRealDeltaTime =
        static_cast<double>(
            elapsedMilliseconds
            )
        / 1000.0;

    constexpr double MAX_REAL_DELTA_TIME =
        0.1;

    const double realDeltaTime =
        std::clamp(
            rawRealDeltaTime,
            0.0,
            MAX_REAL_DELTA_TIME
        );

    const double processDeltaTime =
        realDeltaTime
        * stretchPlaybackSpeed;

    if (!std::isfinite(processDeltaTime)
        || processDeltaTime <= 0.0)
    {
        return;
    }

    controller.advanceDebugStretchBendingPlayback(
        processDeltaTime
    );

    view->update();

    if (
        controller.
        isDebugStretchBendingPlaybackComplete()
        )
    {
        pauseStretchPlayback();

        std::cout
            << "[STRETCH PLAYBACK COMPLETE]"
            << " speed="
            << stretchPlaybackSpeed
            << std::endl;
    }
}
void MainWindow::increaseStretchPlaybackSpeed()
{
    stretchPlaybackSpeed *=
        2.0;

    stretchPlaybackSpeed =
        std::min(
            stretchPlaybackSpeed,
            8.0
        );

    // Restart timing so a speed change cannot reuse an
    // old elapsed interval.
    if (stretchPlaybackRunning)
    {
        stretchPlaybackClock.restart();
    }

    std::cout
        << "[STRETCH PLAYBACK SPEED]"
        << " speed="
        << stretchPlaybackSpeed
        << std::endl;
}

void MainWindow::decreaseStretchPlaybackSpeed()
{
    stretchPlaybackSpeed *=
        0.5;

    stretchPlaybackSpeed =
        std::max(
            stretchPlaybackSpeed,
            0.125
        );

    if (stretchPlaybackRunning)
    {
        stretchPlaybackClock.restart();
    }

    std::cout
        << "[STRETCH PLAYBACK SPEED]"
        << " speed="
        << stretchPlaybackSpeed
        << std::endl;
}

bool MainWindow::
isStretchPlaybackRunning() const
{
    return stretchPlaybackRunning;
}

double MainWindow::
getStretchPlaybackSpeed() const
{
    return stretchPlaybackSpeed;
}