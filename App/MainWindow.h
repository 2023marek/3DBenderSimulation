#pragma once
#include <QMainWindow>
#include <QTimer>
#include "Common/UserAction.h"
#include "AppController.h"
#include "Render/GLView.h"
#include "Render/HUDPanel.h"    

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    
private slots:
    void onUpdate(); // wywo³ywane co frame

private:
    AppController controller;
    HUDPanel* hud;
    GLView* view;
    QTimer timer;
};
  