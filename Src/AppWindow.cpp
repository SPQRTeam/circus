#include "AppWindow.h"

#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <memory>

#include "Constants.h"
#include "Container.h"
#include "MujocoContext.h"
#include "Robot.h"
#include "SceneParser.h"
namespace spqr {

AppWindow::AppWindow(int& argc, char** argv) {
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);

    resize(spqr::initialWindowWidth, spqr::initialWindowHeight);
    setWindowTitle(spqr::appName);

    QWidget* centralWidget = new QWidget;
    mainLayout = new QVBoxLayout;
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    viewportContainer = nullptr;

    QMenu* fileMenu = menuBar()->addMenu("&File");
    QAction* openSceneAction = new QAction("&Open Scene", this);
    fileMenu->addAction(openSceneAction);
    connect(openSceneAction, &QAction::triggered, this, &AppWindow::openScene);

    if (argc > 1) {
        QString fileArg = QString::fromLocal8Bit(argv[1]);
        loadScene(fileArg);
    }
};

void AppWindow::openScene() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Scene File"), "Resources/Scenes/",
                                                    tr("YAML Files (*.yaml)"));
    if (!fileName.isEmpty()) {
        loadScene(fileName);
    }
}

void AppWindow::loadScene(const QString& xml) {
    try {
        if (sim) {
            sim->stop();
            sim.reset();
        }

        if (viewportContainer) {
            mainLayout->removeWidget(viewportContainer);
            viewportContainer->deleteLater();
            viewportContainer = nullptr;
        }

        SceneParser parser(xml.toStdString());
        std::string xmlScene = parser.buildMuJoCoXml();

        mujContext = std::make_unique<MujocoContext>(xmlScene);
        viewport = std::make_unique<SimulationViewport>(*mujContext);

        for (const shared_ptr<Robot>& robot : RobotManager::instance().getRobots()) {
            robot->container = std::make_unique<Container>(robot->name + "_container");
            robot->container->create("ubuntu:22.04", {});
            robot->container->start();
        }

        viewportContainer = QWidget::createWindowContainer(viewport.get());
        mainLayout->addWidget(viewportContainer);

        sim = std::make_unique<SimulationThread>(mujContext->model, mujContext->data);
        sim->start();
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error loading scene", e.what());
    }
}

void AppWindow::signalHandler(int signal) {
    TeamManager::instance().clear();

    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

AppWindow::~AppWindow() {
    if (sim != nullptr && sim->isRunning())
        sim->stop();
    TeamManager::instance().clear();
}
}  // namespace spqr
