#pragma once
#include <mujoco/mujoco.h>

#include <QMainWindow>
#include <QVBoxLayout>
#include <memory>

#include "MujocoContext.h"
#include "SimulationThread.h"
#include "SimulationViewport.h"
namespace spqr {

class AppWindow : public QMainWindow {
   public:
    AppWindow(int& argc, char** argv);
    ~AppWindow();

   private:
    void loadScene(const QString& xml);
    void openScene();

    QVBoxLayout* mainLayout;
    QWidget* viewportContainer;

    std::unique_ptr<MujocoContext> mujContext;
    std::unique_ptr<SimulationViewport> viewport;
    std::unique_ptr<SimulationThread> sim;
};

}  // namespace spqr
