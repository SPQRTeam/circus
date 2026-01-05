#pragma once
#include <mujoco/mujoco.h>
#include <qboxlayout.h>
#include <qqmlapplicationengine.h>
#include <qtmetamacros.h>

#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QVBoxLayout>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

#include "MujocoContext.h"
#include "SimulationThread.h"
#include "SimulationViewport.h"
#include "frontend/ToolsPanel.h"

namespace spqr {

// class AppWindow : public QObject {
//         Q_OBJECT
//         Q_PROPERTY(QString projectRoot READ projectRoot CONSTANT)
//         Q_PROPERTY(QVariantList teams READ getTeamsForQml NOTIFY teamsChanged)

//     public:
//         AppWindow(int& argc, char** argv);
//         ~AppWindow();

//         Q_INVOKABLE void loadScene(const QString& yamlFile);
//         Q_INVOKABLE void updateRobotData();
//         Q_INVOKABLE void pauseSimulation();
//         Q_INVOKABLE void playSimulation();
//         Q_INVOKABLE bool isSimulationPaused() const;

//         QString projectRoot() const {
//             return QString::fromStdString(PROJECT_ROOT);
//         }
//         QVariantList getTeamsForQml() const;

//     signals:
//         void teamsChanged();

//     private:
//         static void signalHandler(int signal);

//         std::unique_ptr<QQmlApplicationEngine> qmlEngine;

//         QWidget* viewportContainer = nullptr;

//         std::unique_ptr<MujocoContext> mujContext;
//         std::unique_ptr<SimulationViewport> viewport;
//         std::unique_ptr<SimulationThread> sim;

//         QList<QObject*> robotWrappers_;
// };

class AppWindow : public QMainWindow {
        Q_OBJECT

    public:
        AppWindow(int& argc, char** argv);
        ~AppWindow();

    private slots:
        void openScene();
        void playSimulation();
        void pauseSimulation();

    private:
        void loadScene(const QString& yaml_file);
        static void signalHandler(int signal);

        QVBoxLayout* mainLayout = nullptr;
        QWidget* viewportContainer = nullptr;
        QLabel* viewportPlaceholder = nullptr;

        ToolsPanel* toolsPanel = nullptr;

        std::unique_ptr<MujocoContext> mujContext;
        std::unique_ptr<SimulationViewport> viewport;
        std::unique_ptr<SimulationThread> sim;
};

}  // namespace spqr
