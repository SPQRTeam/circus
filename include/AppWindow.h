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
