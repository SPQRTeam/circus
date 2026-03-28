# Circus — Issue Report

---

## Issue #1: Disaccoppiamento temporale tra simulation thread e TCP thread

### Contesto

Circus usa due thread che operano in modo completamente asincrono e indipendente:

- **SimulationThread** (`src/SimulationThread.cpp`) — esegue il loop fisico MuJoCo
- **TCP Server Thread** (`src/RobotManager.cpp::_serverInternal`) — gestisce la comunicazione con i container Docker

### Loop di simulazione (SimulationThread.cpp:12-47)

```cpp
while (running_) {
    if (!paused_) {
        mj_step1(model_, data_);          // fase 1: forward dynamics (NO ctrl ancora)
        RobotManager::instance().applyCommands();  // applica ctrl[] ai robot
        mj_step2(model_, data_);          // fase 2: integrazione
        RobotManager::instance().update(); // legge stato post-step → sensor cache
        GameController::instance().update();
        // ...
        std::this_thread::sleep_until(next_step_time);
    }
}
```

Nota: il loop è stato aggiornato rispetto alla prima versione — usa `mj_step1`/`mj_step2` separati con `applyCommands()` in mezzo, il che è corretto dal punto di vista fisico (le torque vengono applicate dopo che le forze di contatto sono già state calcolate in `mj_step1`).

### Loop TCP (RobotManager.cpp:116-269)

```cpp
while (serverRunning_) {
    int ret = poll(fds.data(), fds.size(), 100);  // 100ms timeout
    // ...
    {
        std::unique_lock lock(mutex_);
        r->receiveMessage(data_map);   // scrive latestTorques nel robot (riga 245)
        answ = r->sendMessage();       // legge sensor cache (riga 246)
    }
    // send answ back to container
}
```

### Problema 1a: Disaccoppiamento causale

Il flusso ideale deterministico sarebbe:

```
Container → torques_t → Simulator → mj_step → stato_{t+1} → Container
```

Il flusso reale è:

```
TCP Thread:                         Sim Thread:
recv torques_t                      ...
receiveMessage() → latestTorques    ...
sendMessage() → legge sensor cache  ...  ← quale step?
send stato_?                        applyCommands() → ctrl[]
                                    mj_step1/2
                                    update() → sensor cache
```

Non esiste garanzia che lo stato inviato al container corrisponda allo step che ha usato le torque appena ricevute. Il TCP thread legge la sensor cache immediatamente dopo `receiveMessage()`, che è l'ultimo stato calcolato dal sim thread — che potrebbe essere di 0, 1, o più step fa rispetto all'applicazione delle nuove torque.

### Problema 1b: Assenza di lock su `applyCommands` vs `mj_step`

`applyCommands()` è chiamato dal **SimulationThread** (riga 23 di SimulationThread.cpp):

```cpp
// RobotManager.cpp:9-12
void RobotManager::applyCommands() {
    for (std::shared_ptr<Robot> r : robots_) {
        r->applyCommands();   // ← NESSUN mutex_ qui
    }
}
```

`BoosterT1::applyCommands()` ([robots/BoosterT1.h:166-169](include/robots/BoosterT1.h)):

```cpp
void applyCommands() override {
    std::lock_guard<std::mutex> lock(mutex_);  // ← mutex_ del robot
    joints->set_torque(latestTorques);
}
```

`BoosterT1::receiveMessage()` ([robots/BoosterT1.h:117-138](include/robots/BoosterT1.h)) è chiamato dal **TCP Thread** e scrive in `latestTorques` sotto lo stesso `mutex_` del robot:

```cpp
void receiveMessage(...) override {
    // ...
    std::lock_guard<std::mutex> lock(mutex_);  // ← stesso mutex_ del robot
    for (const auto& [joint_value, joint_name] : joint_map) {
        latestTorques[joint_value] = joint_torques[i++];
    }
}
```

**Il mutex_ del robot protegge correttamente `latestTorques`** da accessi concorrenti tra `receiveMessage()` (TCP thread) e `applyCommands()` (sim thread). Questo è l'unico punto dove la sincronizzazione è corretta.

### Problema 1c: `set_torque` scrive direttamente in `mujData->ctrl[]` senza lock globale

`Joints::set_torque()` ([sensors/Joint.h:98-109](include/sensors/Joint.h)):

```cpp
void set_torque(const std::unordered_map<JointValue, mjtNum>& values) {
    for (const auto& [joint, val] : values) {
        // ...
        mujData->ctrl[act_id] = std::clamp(val, ...);  // scrittura diretta su mjData
    }
}
```

Questa scrittura avviene all'interno di `applyCommands()` che è chiamato dal sim thread **tra** `mj_step1` e `mj_step2`. Questo è corretto per il flusso fisico, ma `mujData` è un puntatore condiviso con tutti gli altri sensori. Non c'è nessun lock globale su `mujData` tra il sim thread e potenziali accessi concorrenti.

### Problema 1d: `RobotManager::applyCommands()` non acquisisce `mutex_` del RobotManager

```cpp
// RobotManager.cpp:9-12
void RobotManager::applyCommands() {
    for (std::shared_ptr<Robot> r : robots_) {  // itera su robots_ SENZA mutex_
        r->applyCommands();
    }
}
```

Tutte le altre operazioni su `robots_` (`update()`, `registerRobot()`, ecc.) acquisiscono `mutex_`. `applyCommands()` no. Se un robot venisse registrato o rimosso durante l'iterazione, ci sarebbe una race condition su `robots_`. In pratica non accade perché i robot vengono registrati prima di avviare i thread, ma è un'inconsistenza.

### Problema 1e: `Sensor::update()` vs `Sensor::serialize()` — shared_mutex parzialmente efficace

`Sensor` ([sensors/Sensor.h](include/sensors/Sensor.h)) usa `shared_mutex`:

```cpp
virtual void update() final {
    std::unique_lock lock(mtx_);  // esclusivo: solo il sim thread
    doUpdate();
}

virtual msgpack::object serialize(msgpack::zone& z) final {
    std::shared_lock lock(mtx_);  // condiviso: lettura concorrente ok
    return doSerialize(z);
}
```

Questo protegge i campi interni del sensore (`position`, `velocity`, ecc.) da race tra `update()` (sim thread) e `serialize()` (TCP thread). **Questo funziona correttamente.**

Tuttavia, `doUpdate()` in `Joints` legge da `mujData` (es. `mujData->qpos`, `mujData->qvel`) e `set_torque()` scrive su `mujData->ctrl[]`. Entrambi accedono a `mujData` ma il `shared_mutex` del sensore **non copre gli accessi a `mujData`** — copre solo i campi cached del sensore.

### Sequenza temporale del problema causale — diagramma

```
Tempo →

SimThread:  [mj_step1]─[applyCommands]─[mj_step2]─[update sensors]  sleep  [mj_step1]─...
                              ↑                          ↓
                       ctrl[] ← latestTorques    sensor cache ← mjData

TCPThread:  ...[recv torques_t]─[receiveMessage]─[sendMessage]─[send]...
                                      ↑                ↓
                               latestTorques       sensor cache (da quale step?)
                               (sarà usato al       (è l'ultimo completato,
                                prossimo            ma non necessariamente quello
                                applyCommands)      che userà torques_t)
```

Lo stato inviato al container dopo `receiveMessage(torques_t)` è lo stato prodotto dallo step che ha usato `torques_{t-k}` per un `k` indeterminato. Il container riceve una risposta che non corrisponde causalmente ai comandi appena inviati.

### Contesto quantitativo

- Sim a `model_->opt.timestep` (tipicamente 1ms → 1000Hz)
- TCP a ~100Hz (poll con timeout 100ms, ma in pratica limitato dalla rete)
- Rapporto: ~10 step di simulazione per ogni ciclo TCP

Con questo rapporto il jitter è al massimo ~10ms, accettabile per molti usi. Diventa rilevante per RL policy che assumono causalità stretta o per debugging deterministico.

### Soluzioni analizzate

#### A — Minimal fix: mutex globale su ctrl[]

Aggiungere un lock su `mujData->ctrl[]` tra sim thread e TCP thread.

- **Pro**: 5 righe di codice
- **Con**: Non risolve la causalità, solo la race condition su ctrl[] (che in realtà non esiste già grazie al mutex_ del robot + applyCommands nel sim thread)
- **Valutazione**: Inutile dato che la race è già gestita

#### B — Double-buffer atomico per robot (raccomandato)

Ogni robot ha due buffer separati scambiati atomicamente: uno per le torque (TCP scrive, sim legge) e uno per lo stato (sim scrive, TCP legge).

```
TCP Thread:                         Sim Thread:
recv torques                        swap torque_pending → ctrl[]
write torque_pending (atomic swap)  mj_step1/2
read state_latest                   update() → write state_latest (atomic swap)
send state
```

```cpp
struct RobotMailbox {
    std::unordered_map<JointValue, mjtNum> torques[2];
    std::atomic<int> torque_write_idx{0};  // TCP scrive in [write_idx], sim legge da [1-write_idx]

    SensorSnapshot state[2];
    std::atomic<int> state_write_idx{0};   // sim scrive in [write_idx], TCP legge da [1-write_idx]
};
```

- **Pro**: Zero blocking, robot completamente indipendenti, jitter ~1ms, sim thread non aspetta mai
- **Con**: No causalità stretta (jitter di 1 step), scambio di buffer richiede breve sincronizzazione sullo swap
- **Valutazione**: Ottimale per questo use case (10 step/ciclo TCP rende il jitter irrilevante)

#### C — Sim-driven non-blocking I/O

Il sim thread gestisce direttamente la rete con `poll(..., timeout=0)` non bloccante ogni step (o ogni N step per matchare 100Hz):

```cpp
// Nel loop di SimulationThread
poll(robot_fds, N, 0);  // non-blocking
for each robot with data: recv torques (MSG_DONTWAIT)
mj_step1();
applyCommands();
mj_step2();
update();
for each robot: send state (MSG_DONTWAIT)
```

- **Pro**: Causalità perfetta, zero sincronizzazione, architettura identica a SimRobot/BHuman
- **Con**: Refactor significativo, dipendenza da velocità rete nel loop fisico, send bloccante se buffer pieno
- **Valutazione**: Architetturalmente più pulito ma richiede gestione errori I/O nel thread critico

#### D — Per-robot condition variable

Il TCP thread aspetta una notifica dopo ogni step prima di rispondere:

```cpp
// TCP Thread per robot i:
receiveMessage();
robot.step_cv.wait(lock, [&]{ return robot.state_ready; });
robot.state_ready = false;
sendMessage();

// Sim Thread dopo update():
for each robot:
    robot.state_ready = true;
    robot.step_cv.notify_one();
```

- **Pro**: Causalità garantita, robot indipendenti
- **Con**: Il TCP thread blocca fino al prossimo step (~1ms max). Se il sim è lento, anche il TCP è lento. Introduce dipendenza temporale tra physics rate e network rate
- **Valutazione**: Valido se la causalità stretta è necessaria (es. training RL)

### Soluzione raccomandata

**Approccio B** per uso generale (sviluppo, test gameplay).
**Approccio D** se si vuole usare Circus per training RL deterministico.

---

## Issue #2: Incompatibilità CMake 4.x con ROS Humble (simbridge)

### Descrizione

Il build dei pacchetti messaggi ROS 2 di simbridge (`booster_interface`, `booster_msgs`, `spqr_msgs`) fallisce quando pixi risolve CMake >= 4.0.

### Causa radice

CMake 4.0 ha rimosso i moduli legacy `FindPythonInterp` e `FindPythonLibs` (policy CMP0148 promossa da WARN a ERROR).

Il file `rosidl_generator_py_generate_interfaces.cmake` di ROS Humble (pacchetto `python_cmake_module`) fa questa sequenza:

```cmake
# rosidl_generator_py_generate_interfaces.cmake:20-35
find_package(python_cmake_module REQUIRED)
find_package(PythonExtra REQUIRED)         # chiama FindPythonExtra → FindPythonInterp (legacy)
find_package(Python REQUIRED               # chiama FindPython moderno
    COMPONENTS Interpreter Development NumPy)
```

Con CMake 4.3 nell'ambiente conda cross-compilation:
- `FindPythonInterp` → funziona (trovato `$PREFIX/bin/python3`) ma genera WARNING CMP0148
- `FindPythonLibs` → funziona ma genera WARNING CMP0148
- `FindPython` → **FALLISCE** perché non trova `Python_NumPy_INCLUDE_DIRS`, `Development.Module`, `Development.Embed`

L'errore esatto:

```
CMake Error: Could NOT find Python (missing: Python_EXECUTABLE Python_INCLUDE_DIRS
Python_LIBRARIES Python_NumPy_INCLUDE_DIRS Interpreter Development NumPy
Development.Module Development.Embed)
at: $BUILD_PREFIX/share/cmake-4.3/Modules/FindPackageHandleStandardArgs.cmake:290
```

Il modulo moderno `FindPython` non riesce a trovare i componenti `Development.Embed` e `Development.Module` nell'ambiente conda cross-compilation perché il `BUILD_PREFIX` e il `PREFIX` sono separati e le librerie Python sono in `PREFIX/lib` ma CMake cerca in percorsi di sistema.

### Tentativi falliti

1. `-DROSIDL_GENERATOR_PY=OFF` — non supportato da `rosidl_cmake`, il generatore Python è sempre caricato
2. `-DCMAKE_POLICY_DEFAULT_CMP0148=OLD` — risolve i warning ma non il `find_package(Python ...)` moderno alla riga 34

### Fix applicata

Pinnato `cmake >=3.20,<4.0` nei recipe dei tre pacchetti:

```yaml
# src/msgs/booster_interface/recipe.yaml
# src/msgs/booster_msgs/recipe.yaml
# src/msgs/spqr_msgs/recipe.yaml
requirements:
  build:
    - cmake >=3.20,<4.0
    - python
    - numpy
    - ros-humble-rosidl-default-generators
  host:
    - ros-humble-rosidl-generator-py
    - python
    - numpy
    - ros-humble-rosidl-default-runtime
```

Con CMake 3.x `FindPython` funziona correttamente nell'ambiente conda perché il modulo legacy conosce i percorsi conda.

### Nota di monitoraggio

ROS Humble ha supporto ufficiale fino a maggio 2027 ma non supporterà mai CMake 4.x. Il problema si ripresenterà se:
- Si migra a ROS Iron/Jazzy (che hanno patch per CMake 4.x)
- Si rimuove il pin `<4.0` dai recipe

Al momento del passaggio a una distro ROS più recente sarà necessario aggiornare i recipe rimuovendo il pin e verificando che `rosidl_generator_py` sia stato patchato per CMake 4.x.
