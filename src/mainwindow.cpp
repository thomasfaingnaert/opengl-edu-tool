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

    // Projection matrix sliders
    ui->projectionNearSlider->init();
    ui->projectionFarSlider->init();
    ui->projectionFovSlider->init(1.0f, 0, "°");

    // Add space display label
    spaceLbl = new QLabel(this);
    spaceLbl->setMargin(10);
    spaceLbl->setText("Huidige ruimte: Model space");
    QGridLayout *lay = static_cast<QGridLayout*>(ui->frame->layout());
    lay->addWidget(spaceLbl, 0, 0, 1, 1, Qt::AlignTop | Qt::AlignLeft);

    // Connect space changed signal
    connect(ui->sceneWidget, &SceneWidget::currentSpaceChanged, this, &MainWindow::onCurrentSpaceChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::onCurrentSpaceChanged(const SceneWidget::Space space)
{
    QString spaceStr;
    switch (space)
    {
    case SceneWidget::Space::Model:
        spaceStr = "Model space";
        break;
    case SceneWidget::Space::World:
        spaceStr = "World space";
        break;
    case SceneWidget::Space::View:
        spaceStr = "View space";
        break;
    case SceneWidget::Space::RenderedImage:
        spaceStr = "Gerenderde afbeelding";
        break;
    default:
        spaceStr = "Onbekend";
        break;
    }

    spaceLbl->setText("Huidige ruimte: " + spaceStr);
}
