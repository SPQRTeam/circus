#pragma once
#include <mujoco/mujoco.h>

#include <QMainWindow>
#include <QVBoxLayout>
#include <memory>

#include "MujocoContext.h"
#include "SimulationThread.h"
#include "SimulationViewport.h"
#include "frontend/CameraSidebar.h"

namespace spqr {

class AppWindow : public QMainWindow {
    public:
        AppWindow(int& argc, char** argv);
        ~AppWindow();

    private:
        void loadScene(const QString& xml);
        void openScene();

        static void signalHandler(int signal);

        QVBoxLayout* mainLayout;
        QWidget* viewportContainer;
        CameraSidebar* cameraSidebar_ = nullptr;

        std::unique_ptr<MujocoContext> mujContext;
        std::unique_ptr<SimulationViewport> viewport;
        std::unique_ptr<SimulationThread> sim;
};

}  // namespace spqr
