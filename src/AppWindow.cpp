#include "AppWindow.h"

#include <qaction.h>

#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <csignal>
#include <string>

#include "Constants.h"
#include "MujocoContext.h"
#include "RobotManager.h"
#include "SceneParser.h"
#include "Team.h"

namespace spqr {

AppWindow::AppWindow(int& argc, char** argv) : QMainWindow() {
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);
    
    std::optional<std::string> scenePath;
    if (argc >= 2 && std::string(argv[1]).ends_with(".yaml")) {
        scenePath = argv[1];
    }

    resize(spqr::initialWindowWidth, spqr::initialWindowHeight);

    mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget* centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #1a1a1a;");
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    if(!scenePath){
        viewportPlaceholder = new QLabel("Circus\nSPQR Team Simulator", this);
        viewportPlaceholder->setAlignment(Qt::AlignCenter);
        viewportPlaceholder->setStyleSheet("QLabel { "
                                           "  color: #666666; "
                                           "  font-size: 24px; "
                                           "  font-weight: bold; "
                                           "  background-color: #0a0a0a; "
                                           "}");
        mainLayout->addWidget(viewportPlaceholder);
    }

    if (toolsPanel) {
        mainLayout->removeWidget(toolsPanel);
        toolsPanel->deleteLater();
        toolsPanel = nullptr;
    }
    toolsPanel = new ToolsPanel(true, this);
    mainLayout->addWidget(toolsPanel);

    // Connect ToolsPanel signals
    connect(toolsPanel, &ToolsPanel::openRequested, this, &AppWindow::openScene);
    connect(toolsPanel, &ToolsPanel::playRequested, this, &AppWindow::playSimulation);
    connect(toolsPanel, &ToolsPanel::pauseRequested, this, &AppWindow::pauseSimulation);
    connect(toolsPanel, &ToolsPanel::resizeDragStarted, this, [this]() {
        if (viewport) {
            viewport->pauseRendering();
        }
    });
    connect(toolsPanel, &ToolsPanel::resizeDragEnded, this, [this]() {
        if (viewport) {
            viewport->resumeRendering();
        }
    });

    if (scenePath) {
        QString fileArg = QString::fromLocal8Bit(scenePath->c_str());
        loadScene(fileArg);
    }
}

void AppWindow::openScene() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Scene File"), "resources/scenes/", tr("YAML Files (*.yaml)"));
    if (!fileName.isEmpty()) {
        loadScene(fileName);
    }
}

void AppWindow::loadScene(const QString& yaml_file) {
    try {
        TeamManager::instance().clear();
        RobotManager::instance().stopCommunicationServer();

        if (sim) {
            sim->stop();
            sim.reset();
        }

        if (viewportContainer) {
            mainLayout->removeWidget(viewportContainer);
            viewportContainer->deleteLater();
            viewportContainer = nullptr;
        }

        // Hide placeholder if visible
        if (viewportPlaceholder && viewportPlaceholder->isVisible()) {
            viewportPlaceholder->hide();
        }

        SceneParser parser(yaml_file.toStdString());
        std::string xmlScene = parser.buildMuJoCoXml();

        mujContext = std::make_unique<MujocoContext>(xmlScene);
        viewport = std::make_unique<SimulationViewport>(*mujContext);

        RobotManager::instance().startContainers();
        RobotManager::instance().bindMujoco(mujContext.get());

        if (toolsPanel) {
            mainLayout->removeWidget(toolsPanel);
            toolsPanel->deleteLater();
            toolsPanel = nullptr;
        }

        viewportContainer = QWidget::createWindowContainer(viewport.get());
        viewportContainer->setParent(nullptr);
        viewportContainer->setWindowFlags(Qt::Widget);
        mainLayout->addWidget(viewportContainer);

        toolsPanel = new ToolsPanel(false, this);
        mainLayout->addWidget(toolsPanel);

        // Reconnect ToolsPanel signals
        connect(toolsPanel, &ToolsPanel::openRequested, this, &AppWindow::openScene);
        connect(toolsPanel, &ToolsPanel::playRequested, this, &AppWindow::playSimulation);
        connect(toolsPanel, &ToolsPanel::pauseRequested, this, &AppWindow::pauseSimulation);
        connect(toolsPanel, &ToolsPanel::resizeDragStarted, this, [this]() {
            if (viewport) {
                sim->pause();
                viewport->pauseRendering();
            }
        });
        connect(toolsPanel, &ToolsPanel::resizeDragEnded, this, [this]() {
            if (viewport) {
                viewport->resumeRendering();
                sim->play();
            }
        });

        sim = std::make_unique<SimulationThread>(mujContext->model, mujContext->data);
        sim->start();

        // Set initial simulation state (playing when scene is loaded)
        toolsPanel->setSimulationPlaying(true);

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Error loading scene: %1").arg(e.what()));
    } catch (...) {
        QMessageBox::critical(this, "Error", "Unknown error loading scene");
    }
}

void AppWindow::playSimulation() {
    if (sim && sim->isRunning()) {
        sim->play();
        if (toolsPanel) {
            toolsPanel->setSimulationPlaying(true);
        }
    }
}

void AppWindow::pauseSimulation() {
    if (sim && sim->isRunning()) {
        sim->pause();
        if (toolsPanel) {
            toolsPanel->setSimulationPlaying(false);
        }
    }
}

void AppWindow::signalHandler(int signal) {
    TeamManager::instance().clear();
    RobotManager::instance().stopCommunicationServer();

    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

AppWindow::~AppWindow() {
    if (sim != nullptr && sim->isRunning())
        sim->stop();
    TeamManager::instance().clear();
    RobotManager::instance().stopCommunicationServer();
}

}  // namespace spqr
