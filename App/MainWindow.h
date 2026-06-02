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
protected:
    void keyPressEvent(QKeyEvent* event) override;
private slots:
    void onUpdate(); 

private:
    AppController controller;
    HUDPanel* hud;
    GLView* view;
    QTimer timer;
};
  