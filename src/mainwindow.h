#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "scenewidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void onCurrentSpaceChanged(const SceneWidget::Space space);

private:
    Ui::MainWindow *ui;
    QLabel *spaceLbl;
};

#endif // MAINWINDOW_H
