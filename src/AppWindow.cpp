#include "AppWindow.h"

#include <qaction.h>
#include <unistd.h>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#ifdef __GNUC__
#include <cxxabi.h>
#include <execinfo.h>
#endif

#include "CircusNetwork.h"
#include "CommandConsole.h"
#include "Constants.h"
#include "GameControllerBridge.h"
#include "MujocoContext.h"
#include "RobotManager.h"
#include "SceneParser.h"
#include "Team.h"
#include "frontend/game_controller_panel_column/GameControllerPanelColumnContainer.h"
#include "frontend/game_controller_panel_header/GameControllerPanelHeaderContainer.h"

namespace spqr {

AppWindow::AppWindow(int& argc, char** argv) : QMainWindow() {
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGSEGV, signalHandler);
    std::signal(SIGABRT, signalHandler);

    std::optional<std::string> scenePath;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--scene" && i + 1 < argc)
            scenePath = argv[++i];
        else if (arg == "--framework" && i + 1 < argc)
            frameworkConfigPath_ = argv[++i];
        else if (arg == "--paths" && i + 1 < argc)
            pathsConfigPath_ = argv[++i];
        else if (arg == "--connect-mode" && i + 1 < argc)
            connectMode_ = argv[++i];  // TODO: forse si può aggiungere come variabile d'ambiente??
        else if (arg == "--no-images")
            publishImages_ = false;
    }

    resize(spqr::initialWindowWidth, spqr::initialWindowHeight);

    mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget* centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #1a1a1a;");
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    // Create horizontal layout for GameControllerPanelColumnContainer and viewport
    contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    QWidget* contentWidget = new QWidget(this);
    contentWidget->setLayout(contentLayout);
    mainLayout->addWidget(contentWidget);

    toolsPanelSpace = new QWidget(this);
    mainLayout->addWidget(toolsPanelSpace);
    centralWidget->installEventFilter(this);

    if (!scenePath) {
        viewportPlaceholder = new QLabel("Circus\nSPQR Team Simulator", this);
        viewportPlaceholder->setAlignment(Qt::AlignCenter);
        viewportPlaceholder->setStyleSheet("QLabel { "
                                           "  color: #666666; "
                                           "  font-size: 24px; "
                                           "  font-weight: bold; "
                                           "  background-color: #0a0a0a; "
                                           "}");
        contentLayout->addWidget(viewportPlaceholder);
    }

    if (toolsPanel) {
        mainLayout->removeWidget(toolsPanel);
        toolsPanel->deleteLater();
        toolsPanel = nullptr;
    }
    toolsPanel = new ToolsPanel(true, *mujContext, this);
    attachToolsPanel();

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

// The tools panel is not part of the layout: it lies over the bottom of the window, so opening it
// covers the 3D view instead of shrinking it. The layout only keeps room for its collapsed header.
void AppWindow::attachToolsPanel() {
    toolsPanel->setParent(centralWidget());
    // The viewport is a native window, which is drawn above every non-native sibling.
    toolsPanel->setAttribute(Qt::WA_NativeWindow);
    toolsPanel->installEventFilter(this);
    toolsPanel->show();
    placeToolsPanel();
}

void AppWindow::placeToolsPanel() {
    if (!toolsPanel || !centralWidget()) {
        return;
    }
    QWidget* central = centralWidget();
    toolsPanelSpace->setFixedHeight(toolsPanel->collapsedHeight());
    int height = toolsPanel->height();
    toolsPanel->setGeometry(0, central->height() - height, central->width(), height);
    toolsPanel->raise();
}

bool AppWindow::eventFilter(QObject* watched, QEvent* event) {
    if ((watched == centralWidget() || watched == toolsPanel) && event->type() == QEvent::Resize) {
        placeToolsPanel();
    }
    return QMainWindow::eventFilter(watched, event);
}

void AppWindow::openScene() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Scene File"), "resources/scenes/", tr("YAML Files (*.yaml)"));
    if (!fileName.isEmpty()) {
        loadScene(fileName);
    }
}

void AppWindow::cleanupSharedMemory() {
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path(sharedMemoryPath), ec);
    if (ec) {
        std::cerr << "Failed to clean " << sharedMemoryPath << ": " << ec.message() << std::endl;
    }
}

void AppWindow::loadScene(const QString& yaml_file) {
    try {
        TeamManager::instance().clear();

        if (sim) {
            sim->stop();
            sim.reset();
        }

        if (viewportContainer) {
            contentLayout->removeWidget(viewportContainer);
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
        viewport->setCamerasEnabled(publishImages_);

        // Configure and bind the official GameController and the sidebar console
        GameControllerBridge::instance().configure(parser.getSceneInfo().simulationConfig);
        GameControllerBridge::instance().bindMujoco(mujContext.get());
        CommandConsole::instance().configure(parser.getSceneInfo().simulationConfig);
        CommandConsole::instance().bindMujoco(mujContext.get());

        // Recreate GameControllerPanelHeaderContainer
        if (gameControllerPanelHeaderContainer) {
            mainLayout->removeWidget(gameControllerPanelHeaderContainer);
            gameControllerPanelHeaderContainer->deleteLater();
        }
        gameControllerPanelHeaderContainer = new GameControllerPanelHeaderContainer(this);
        mainLayout->insertWidget(0, gameControllerPanelHeaderContainer);

        // Recreate GameControllerPanelColumnContainer with the updated console
        if (gameControllerPanelColumnContainer) {
            contentLayout->removeWidget(gameControllerPanelColumnContainer);
            gameControllerPanelColumnContainer->deleteLater();
        }
        gameControllerPanelColumnContainer = new GameControllerPanelColumnContainer(this);
        contentLayout->insertWidget(0, gameControllerPanelColumnContainer);

        if (toolsPanel) {
            mainLayout->removeWidget(toolsPanel);
            toolsPanel->deleteLater();
            toolsPanel = nullptr;
        }

        viewportContainer = QWidget::createWindowContainer(viewport.get());
        viewportContainer->setParent(nullptr);
        viewportContainer->setWindowFlags(Qt::Widget);

        // Add viewport with margins and spacing
        contentLayout->addWidget(viewportContainer);
        contentLayout->setContentsMargins(0, 0, 5, 0);
        contentLayout->setSpacing(5);  // Space between GameControllerPanelColumnContainer and viewport

        toolsPanel = new ToolsPanel(false, *mujContext, this);
        attachToolsPanel();

        mainLayout->setSpacing(5);  // Space between viewport area and the room of the toolsPanel

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
        sim->setMaxSimulationTime(parser.getSceneInfo().simulationConfig.simulation.max_simulation_time);
        sim->setPublishImages(publishImages_);
        connect(sim.get(), &SimulationThread::maxSimulationTimeReached, this, &AppWindow::close);

        connect(
            sim.get(), &SimulationThread::allRobotsReadySignal, this,
            [this]() {
                if (sim) {
                    std::cout << "Starting simulation!" << std::endl;
                    sim->start();
                }
            },
            Qt::QueuedConnection);

        // Ensure the shared memory directory exists and is writable by the current user.
        // Docker bind mounts create missing host dirs as root, so remove and recreate if needed.
        if (connectMode_ == "shm") {
            const std::filesystem::path shmDir(sharedMemoryPath);
            cleanupSharedMemory();

            std::error_code ec;
            std::filesystem::create_directories(shmDir, ec);
            if (ec || access(shmDir.c_str(), W_OK) != 0) {
                throw std::runtime_error(shmDir.string() + " is not writable by the current user. Remove it (sudo rm -rf " + shmDir.string()
                                         + ") and load the scene again.");
            }

            CircusNetwork::instance().init();
            RobotManager::instance().bindMujoco(mujContext.get(), connectMode_);  // memo: this must be run before starting the communications server

        } else {  // socket mode

            CircusNetwork::instance().init();
            RobotManager::instance().bindMujoco(mujContext.get(), connectMode_);  // memo: this must be run before starting the communications server
            RobotManager::instance().initializeSocket(frameworkCommunicationPort);
        }

        std::cout << "Starting containers..." << std::endl;
        RobotManager::instance().startContainers(frameworkConfigPath_, pathsConfigPath_, connectMode_);

        std::cout << "Simulation starting with " << connectMode_ << " connection mode..." << std::endl;
        std::cout << "Camera images: " << (publishImages_ ? "ENABLED" : "DISABLED (--no-images)") << std::endl;
        std::cout << "Connecting Robots..." << std::endl;
        RobotManager::instance().waitRobotConnections();

        std::cout << "Waiting Robots are Ready..." << std::endl;
        RobotManager::instance().receiveCommandMessages();

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
    // Get signal name
    const char* signalName = "UNKNOWN";
    switch (signal) {
        case SIGSEGV:
            signalName = "SIGSEGV (Segmentation fault)";
            break;
        case SIGABRT:
            signalName = "SIGABRT (Abort)";
            break;
        case SIGTERM:
            signalName = "SIGTERM (Terminated)";
            break;
        case SIGINT:
            signalName = "SIGINT (Interrupt)";
            break;
        case SIGFPE:
            signalName = "SIGFPE (Floating point exception)";
            break;
        case SIGILL:
            signalName = "SIGILL (Illegal instruction)";
            break;
        case SIGBUS:
            signalName = "SIGBUS (Bus error)";
            break;
    }

    std::cerr << "\n========================================" << std::endl;
    std::cerr << "[CRASH] Signal received: " << signalName << std::endl;
    std::cerr << "========================================" << std::endl;

#ifdef __GNUC__
    // Print backtrace
    std::cerr << "\nBacktrace:" << std::endl;
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** symbols = backtrace_symbols(callstack, frames);

    if (symbols) {
        for (int i = 0; i < frames; ++i) {
            // Try to demangle C++ symbols
            char* mangled = nullptr;
            char* offset_begin = nullptr;
            char* offset_end = nullptr;

            // Find parentheses and +address offset surrounding mangled name
            for (char* p = symbols[i]; *p; ++p) {
                if (*p == '(') {
                    mangled = p + 1;
                } else if (*p == '+') {
                    offset_begin = p;
                } else if (*p == ')' && offset_begin) {
                    offset_end = p;
                    break;
                }
            }

            if (mangled && offset_begin && offset_end && mangled < offset_begin) {
                *offset_begin = '\0';
                int status;
                char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
                if (status == 0 && demangled) {
                    std::cerr << "  [" << i << "] " << demangled << " +" << (offset_begin + 1) << std::endl;
                    std::free(demangled);
                } else {
                    std::cerr << "  [" << i << "] " << symbols[i] << std::endl;
                }
            } else {
                std::cerr << "  [" << i << "] " << symbols[i] << std::endl;
            }
        }
        std::free(symbols);
    }
    std::cerr << std::endl;
#endif

    std::cerr << "Cleaning up resources..." << std::endl;
    std::cerr.flush();

    TeamManager::instance().clear();
    cleanupSharedMemory();

    std::cerr << "Cleanup complete. Re-raising signal." << std::endl;
    std::cerr.flush();

    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

AppWindow::~AppWindow() {
    if (sim != nullptr && sim->isRunning())
        sim->stop();
    TeamManager::instance().clear();
    cleanupSharedMemory();
}

}  // namespace spqr
