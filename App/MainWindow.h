#pragma once
#include <QMainWindow>
#include <QTimer>
#include "AppController.h"
 #include "Render/GLView.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private slots:
    void onUpdate(); // wywo³ywane co frame

private:
    AppController controller;
    GLView* view;
    QTimer timer;
};
