#include "SimulationThread.h"

#include <stdexcept>

#include "GameController.h"
#include "RobotManager.h"

namespace spqr {

SimulationThread::SimulationThread(const mjModel* model, mjData* data) : model_(model), data_(data), running_(true), paused_(false) {
    RobotManager::instance().setAreAllRobotsReadyCallback([this]() { emit allRobotsReadySignal(); });
}

/*
//  SimulationThread IDEA
    mj_step1();
    applyCommands();
    mj_step2();
    update();
    // ogni N step (per matchare ~100Hz):
    for each robot:
        send(state)       // non-blocking
        recv(torques)     // bloccante o con timeout
*/

void SimulationThread::run() {
    if (!model_)
        throw std::runtime_error("Cannot start simulation without mujoco model");

    // Qui viene spiegato il funzionamento del ciclo di simulazione, poiché si deve interfacciare
    // con il framework e con la sua frequenza di update (del nodo Brain)
    // Parametri:
    //      kControlDecimation  = numero di step da eseguire per ogni comando desiderato che arriva dal framework (posizione dei giunti desiderata)
    //      kTimestepPolicy     = tempo (in secondi) corrispondente alla frequenza del nodo Brain del framework 
    //                            (al momento 0.02 perché ho settato la frequenza di update del nodo Brain a 50Hz, quindi 1/50 = 0.02)
    //      sim_dt              = dt di quanto viene incrementata la simulazione ogni volta che viene chiamato mj_step (viene preso da SceneParser)
    //
    // Parametri del framework:
    //      POLICY_DT           = quanto viene aumentata la fase del gait ogni volta che viene pubblicato un comando (vale per le policy custom,
    //                            e sarebbe buona pratica essere uguale "kTimestepPolicy", ma possono anche differire. La consequenza sarebbe una 
    //                            simulazione più lenta della realtà). Si trova al momento in Locomotion.h
    //
    // L'idea del funzionamento è la seguente:
    // per ogni comando desiderato (che rappresenta la posizione desiderata dei giunti), l'idea è che quel comando verrà applicato più di una volta. 
    // In particolare, lo applicheremo "kControlDecimation" volte, e deve valere la relazione per cui:
    //      POLICY_DT = kControlDecimation * sim_dt
    // In questo modo, ogni volta che applichiamo un comando ad alto livello, teniamo conto che l'avanzamento totale della simulazione sarà di "POLICY_DT".
    // 
    // Invece, ogni iterazione del ciclo interno "kControlDecimation" trasforma i comandi ad alto livello in torques a basso livello da dover
    // applicare alla simulazione per farla avanzare di un solo "sim_dt". Quando questi comandi sono stati applicati e la simulazione è avanzata,
    // il nuovo stato della simulazione deve essere inviato per poter calcolare i nuovi comandi a basso livello (i.e. "sendStateMessages"), e questi
    // devono poter essere ricevuti (i.e. "receiveCommandMessages"). 
    // In pratica, il loop "for" funziona come se ci fosse un PID ad alta frequenza che calcola le torques a partire dalle joints desiderate e dai
    // joint states attuali.
    //
    // Una volta terminati tutti i "kControlDecimation" steps, addormenta il thread finché il nodo di Brain non è pronto a mandare un
    // nuovo comando desiderato.
    //
    // N.B.1:  in questa pipeline, è molto importante che il tempo di computazione dei "kControlDecimation" steps sia inferiore a "kTimestepPolicy",
    //        perché altrimenti il nodo di Brain manda un nuovo comando desiderato mentre si sta ancora aggiornando la simulazione con il comando
    //        desiderato precedente. 
    //
    // N.B.2: POLICY_DT e kTimestepPolicy possono potenzialmente differire, il che significa che ogni volta che il nodo di Brain invia un comando 
    //        desiderato (cioè ogni "kTimestepPolicy" secondi), il comando è quello giusto per far avanzare la simulazione di "POLICY_DT" secondi
    //
    // N.B.3: il bottleneck al momento sembra essere sendStateMessages+receiveCommandMessages; aumentando il numero di kControlDecimation steps,
    //        ovviamente il sistema rallenta. Quello che non deve succedere è quello spiegato in N.B.1
    //
    // NOTE: con la shm funziona più o meno in real-time. Usando la socket, conviene impostare il seguente profilo al momento:
    //              constexpr int kControlDecimation = 5;
    //              constexpr double kTimestepPolicy = 0.2;
    //              timestep (in SceneParser) = 0.004;
    //              update_rate (nodo Brain)= 5;
    
    double sim_dt = model_->opt.timestep;

    constexpr int kControlDecimation = 10;
    constexpr double kTimestepPolicy = 0.02;
    int stepsSinceLastControl = 0;

    using clock = std::chrono::steady_clock;
    auto next_step_time = clock::now();
    while (running_) {
        if (!paused_) {
            // DEBUG
            // auto stepsStart = clock::now();
            
            for(int i=0; i<kControlDecimation; ++i){
                mj_step1(model_, data_);
                RobotManager::instance().applyCommands();
                mj_step2(model_, data_);
                RobotManager::instance().update();
                GameController::instance().update();

                std::memset(data_->xfrc_applied, 0, model_->nbody * 6 * sizeof(mjtNum));

                if (maxSimulationTime_ > 0 && data_->time >= maxSimulationTime_) {
                    running_ = false;
                    emit maxSimulationTimeReached();
                    break;
                }
    
                RobotManager::instance().sendStateMessages();
                RobotManager::instance().receiveCommandMessages();
                
            }
            // DEBUG: real time spent computing kControlDecimation physics steps, vs. the sim-time budget they represent.
            // double stepsElapsedMs = std::chrono::duration<double, std::milli>(clock::now() - stepsStart).count();
            // double stepsBudgetMs = kControlDecimation * sim_dt * 1000.0;
            // std::cout << "[SimulationThread] " << kControlDecimation << " physics steps took "
            //           << stepsElapsedMs << " ms (sim-time budget " << stepsBudgetMs << " ms)" << std::endl;

            next_step_time += std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(kTimestepPolicy));
            std::this_thread::sleep_until(next_step_time);

            if (clock::now() >= next_step_time)
                next_step_time = clock::now();
        } else {
            // When paused, sleep briefly to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // Reset next_step_time when paused to avoid catching up when playd
            next_step_time = clock::now();
        }
    }
}

void SimulationThread::stop() {
    running_ = false;
    wait();
}

void SimulationThread::pause() {
    paused_ = true;
}

void SimulationThread::play() {
    paused_ = false;
}

bool SimulationThread::isPaused() {
    return paused_;
}

void SimulationThread::setMaxSimulationTime(int maxTime) {
    maxSimulationTime_ = maxTime;
}

}  // namespace spqr
