#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Model matrix sliders
    ui->modelScaleXSlider->init();
    ui->modelScaleYSlider->init();
    ui->modelScaleZSlider->init();

    ui->modelRotateXSlider->init(1.0f, 0, "°");
    ui->modelRotateYSlider->init(1.0f, 0, "°");
    ui->modelRotateZSlider->init(1.0f, 0, "°");

    ui->modelTranslateXSlider->init();
    ui->modelTranslateYSlider->init();
    ui->modelTranslateZSlider->init();

    // View matrix sliders
    ui->viewPositionXSlider->init();
    ui->viewPositionYSlider->init();
    ui->viewPositionZSlider->init();

    ui->viewTargetXSlider->init();
    ui->viewTargetYSlider->init();
    ui->viewTargetZSlider->init();

    ui->viewUpXSlider->init();
    ui->viewUpYSlider->init();
    ui->viewUpZSlider->init();
}

MainWindow::~MainWindow()
{
    delete ui;
}
