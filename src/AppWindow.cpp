// #include "AppWindow.h"

// #include <qdebug.h>

// #include <QDebug>
// #include <QFileDialog>
// #include <QMenuBar>
// #include <QMessageBox>
// #include <QQmlContext>
// #include <QQuickItem>
// #include <QQuickWindow>
// #include <QTimer>
// #include <QWindow>
// #include <QtQml>
// #include <csignal>

// #include "MujocoContext.h"
// #include "RobotManager.h"
// #include "RobotQmlWrapper.h"
// #include "SceneParser.h"
// #include "Team.h"
// #include "ToolCellWrapper.h"

// namespace spqr {

// QVariantList AppWindow::getTeamsForQml() const {
//     QVariantList teamsList;

//     auto teams = TeamManager::instance().getTeams();

//     for (const auto& team : teams) {
//         QVariantMap teamMap;
//         teamMap["name"] = QString::fromStdString(team->name);

//         QVariantList robotsList;
//         for (const auto& robot : team->robots) {
//             auto robotWrapper = new RobotQmlWrapper(robot, const_cast<AppWindow*>(this));
//             const_cast<AppWindow*>(this)->robotWrappers_.append(robotWrapper);
//             robotsList.append(QVariant::fromValue(robotWrapper));
//         }
//         teamMap["robots"] = robotsList;
//         teamsList.append(teamMap);
//     }

//     return teamsList;
// }

// void AppWindow::updateRobotData() {
//     // Call update() on all robot wrappers to emit signals
//     for (QObject* obj : robotWrappers_) {
//         if (auto wrapper = qobject_cast<RobotQmlWrapper*>(obj)) {
//             wrapper->update();
//         }
//     }
// }

// AppWindow::AppWindow(int& argc, char** argv) {
//     std::signal(SIGTERM, signalHandler);
//     std::signal(SIGINT, signalHandler);
//     std::signal(SIGSEGV, signalHandler);
//     std::signal(SIGABRT, signalHandler);

//     // Register custom QML types
//     qmlRegisterType<ToolCellWrapper>("Circus", 1, 0, "ToolCellWrapper");

//     // Create QML engine ONCE
//     qmlEngine = std::make_unique<QQmlApplicationEngine>();
//     qmlEngine->rootContext()->setContextProperty("appWindow", this);

//     const QUrl qrcUrl = QUrl(QStringLiteral("resources/qml/main.qml"));

//     // Connect to objectCreated to detect if QML failed to load
//     QObject::connect(qmlEngine.get(), &QQmlApplicationEngine::objectCreated, [](QObject* obj, const QUrl& objUrl) {
//         if (!obj) {
//             qCritical() << "Failed to load QML:" << objUrl;
//             QCoreApplication::exit(-1);
//         }
//     });

//     qmlEngine->load(qrcUrl);

//     // Load scene from command line argument if provided
//     std::optional<std::string> scenePath;
//     if (argc >= 2 && std::string(argv[1]).ends_with(".yaml")) {
//         scenePath = argv[1];
//     }
//     if (scenePath) {
//         QString fileArg = QString::fromLocal8Bit(scenePath->c_str());
//         QTimer::singleShot(100, [this, fileArg]() { loadScene(fileArg); });
//     }
// };

// void AppWindow::loadScene(const QString& yamlFile) {
//     try {
//         qDebug() << "Loading scene:" << yamlFile;

//         qDebug() << "1. Clearing team manager...";
//         TeamManager::instance().clear();

//         qDeleteAll(robotWrappers_);
//         robotWrappers_.clear();

//         qDebug() << "2. Stopping communication server...";
//         RobotManager::instance().stopCommunicationServer();

//         if (sim) {
//             qDebug() << "3. Stopping simulation...";
//             sim->stop();
//             sim.reset();
//         }

//         if (viewportContainer) {
//             qDebug() << "4. Cleaning up old viewport...";
//             viewportContainer->deleteLater();
//             viewportContainer = nullptr;
//         }

//         if (viewport) {
//             qDebug() << "4b. Cleaning up viewport window...";
//             viewport.reset();
//         }

//         qDebug() << "5. Parsing scene...";
//         SceneParser parser(yamlFile.toStdString());
//         std::string xmlScene = parser.buildMuJoCoXml();

//         qDebug() << "6. Creating MuJoCo context...";
//         mujContext = std::make_unique<MujocoContext>(xmlScene);

//         qDebug() << "7. Creating viewport...";
//         viewport = std::make_unique<SimulationViewport>(*mujContext);

//         qDebug() << "8. Starting containers...";
//         RobotManager::instance().startContainers();

//         qDebug() << "9. Binding MuJoCo...";
//         RobotManager::instance().bindMujoco(mujContext.get());

//         qDebug() << "10. Embedding viewport in QML...";

//         if (qmlEngine->rootObjects().isEmpty()) {
//             qWarning() << "No root QML objects found!";
//             return;
//         }

//         QObject* rootObject = qmlEngine->rootObjects().first();
//         QQuickItem* qmlContainer = rootObject->findChild<QQuickItem*>("viewportContainer");

//         if (qmlContainer) {
//             QQuickWindow* quickWindow = qmlContainer->window();

//             if (quickWindow) {
//                 viewport->setTransientParent(quickWindow);
//                 viewport->setFlags(Qt::Widget);
//                 viewportContainer = QWidget::createWindowContainer(viewport.get());
//                 viewportContainer->setParent(nullptr);
//                 viewportContainer->setWindowFlags(Qt::Widget);
//                 WId viewportWinId = viewportContainer->winId();
//                 WId quickWinId = quickWindow->winId();

// // On X11, we can directly set the parent using xcb
// #ifdef Q_OS_LINUX
//                 viewportContainer->windowHandle()->setParent(quickWindow);
// #endif

//                 auto updateGeometry = [qmlContainer, quickWindow, this]() {
//                     QPointF scenePos = qmlContainer->mapToScene(QPointF(0, 0));
//                     QPoint globalPos = quickWindow->mapToGlobal(scenePos.toPoint());

//                     this->viewportContainer->setGeometry(scenePos.x(), scenePos.y(), qmlContainer->width(), qmlContainer->height());
//                 };

//                 updateGeometry();

//                 QObject::connect(qmlContainer, &QQuickItem::widthChanged, updateGeometry);
//                 QObject::connect(qmlContainer, &QQuickItem::heightChanged, updateGeometry);
//                 QObject::connect(qmlContainer, &QQuickItem::xChanged, updateGeometry);
//                 QObject::connect(qmlContainer, &QQuickItem::yChanged, updateGeometry);

//                 viewportContainer->show();
//                 viewportContainer->raise();

//                 qDebug() << "Viewport embedded successfully";
//             } else {
//                 qWarning() << "Could not get QQuickWindow";
//             }
//         } else {
//             qWarning() << "Could not find viewportContainer in QML";
//         }

//         qDebug() << "12. Starting simulation thread...";
//         sim = std::make_unique<SimulationThread>(mujContext->model, mujContext->data);

//         // Connect simulation step to update robot data
//         connect(sim.get(), &SimulationThread::stepCompleted, this, &AppWindow::updateRobotData, Qt::QueuedConnection);

//         sim->start();

//         qDebug() << "13. Emitting teams changed signal...";
//         emit teamsChanged();

//         qDebug() << "Scene loaded successfully";
//     } catch (const std::exception& e) {
//         qWarning() << "Error loading scene:" << e.what();
//     } catch (...) {
//         qWarning() << "Unknown error loading scene";
//     }
// }

// void AppWindow::pauseSimulation() {
//     if (sim && sim->isRunning()) {
//         sim->pause();
//         qDebug() << "Simulation paused";
//     }
// }

// void AppWindow::playSimulation() {
//     if (sim && sim->isRunning()) {
//         sim->play();
//         qDebug() << "Simulation playd";
//     }
// }

// bool AppWindow::isSimulationPaused() const {
//     if (sim && sim->isRunning()) {
//         return sim->isPaused();
//     }
//     return false;
// }

// void AppWindow::signalHandler(int signal) {
//     TeamManager::instance().clear();
//     RobotManager::instance().stopCommunicationServer();

//     std::signal(signal, SIG_DFL);
//     std::raise(signal);
// }

// AppWindow::~AppWindow() {
//     if (sim != nullptr && sim->isRunning())
//         sim->stop();
//     TeamManager::instance().clear();
//     RobotManager::instance().stopCommunicationServer();
// }
// }  // namespace spqr

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

    resize(spqr::initialWindowWidth, spqr::initialWindowHeight);

    mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 5);

    QWidget* centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #1a1a1a;");
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    // Create viewport placeholder
    viewportPlaceholder = new QLabel("Circus\nSPQR Team Simulator", this);
    viewportPlaceholder->setAlignment(Qt::AlignCenter);
    viewportPlaceholder->setStyleSheet("QLabel { "
                                       "  color: #666666; "
                                       "  font-size: 24px; "
                                       "  font-weight: bold; "
                                       "  background-color: #0a0a0a; "
                                       "}");
    mainLayout->addWidget(viewportPlaceholder);

    // Create ToolsPanel at startup
    toolsPanel = new ToolsPanel(this);
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

    std::optional<std::string> scenePath;
    if (argc >= 2 && std::string(argv[1]).ends_with(".yaml")) {
        scenePath = argv[1];
    }

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

        toolsPanel = new ToolsPanel(this);
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
