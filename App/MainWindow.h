#pragma once
#include <cmath>
#include <algorithm>
#include <QElapsedTimer>
#include <QTimer>
#include <QMainWindow>
#include "Common/UserAction.h"
#include "AppController.h"
#include "Render/GLView.h"
#include "Render/HUDPanel.h" 
         

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
protected:
    void keyPressEvent(QKeyEvent* event) override;
private slots:
    void onUpdate(); 

private:
    AppController controller;
    HUDPanel* hud;
    GLView* view;
    QTimer timer;
private:
    // Periodically requests automatic stretch-playback
    // advancement.
    QTimer stretchPlaybackTimer;

    // Measures real elapsed time between timer callbacks.
    //
    // QTimer intervals are not guaranteed to be exact,
    // so elapsed time should be measured rather than
    // assuming every callback is exactly 16 ms.
    QElapsedTimer stretchPlaybackClock;

    bool stretchPlaybackRunning =
        false;

    // 1.0 = normal process time
    // 0.5 = half speed
    // 2.0 = double speed
    double stretchPlaybackSpeed =
        1.0;

private:
    void toggleStretchPlayback();

    void startStretchPlayback();

    void pauseStretchPlayback();

    void updateStretchPlayback();

    void increaseStretchPlaybackSpeed();

    void decreaseStretchPlaybackSpeed();
};
  