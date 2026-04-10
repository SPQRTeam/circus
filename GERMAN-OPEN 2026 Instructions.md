# GERMAN-OPEN 2026 Instructions

Questa è la guida per far funzionare la simulazione con il framework spqrbooster2026.

I seguenti passaggi vanno eseguiti in ordine per setuppare tutto

## Simbridge
0) Assicurati di essere nel branch `dev`
1) Lancia il comando `pixi install`. Non serve fare altro

## Maximus
0) Assicurati che in `config/framework.yaml` ci sia la configurazione giusta di topic e moduli
1) Assciurati che in `src/app/pixi.toml` ci sia la versione di `booster_robotics_sdk` corretta
2) Lancia il comando `pixi install` per installare l'ambiente
3) Lancia il comando `pixi run main` per buildare e lanciare il programma

## Circus
1) Assicurati di essere nel branch `oracle`
2) Clona dentro la cartella `dockerfiles` le repo di booster:
```
cd dockerfiles
git clone https://github.com/BoosterRobotics/booster_robotics_sdk.git
git clone https://github.com/BoosterRobotics/booster_robotics_sdk_ros2.git
```
PS: questo step non è realmente necessario per il funzionamento di tutto, serve per poter interagire con i messaggi ros dentro al docker. Se non volete clonare le repo, dovete modificare il dockerFile per non eseguire le istruzioni a loro collegate. Secondo me per il momento conviene farlo.

3) Builda il docker:
```
cd dockerfiles
docker build -t spqr:booster .
```

4) Dentro la cartella `resources` crea il file `path_constants.yaml` inserendo i path assoluti del tuo pc dei seguenti componenti:
```
maximus: /home/flavio/Scrivania/RoboCup/spqrbooster2026
circus: /home/flavio/Scrivania/RoboCup/circus
booster_robotics_sdk: /home/flavio/Scrivania/RoboCup/circus/dockerfiles/booster_robotics_sdk
simbridge: /home/flavio/Scrivania/RoboCup/simbridge
```

5) Istalla l'ambiente con il comando `pixi install`
6) Lancia la simulazione con il comando `pixi run circus`


## Dopo l'istallazione

Una volta che avete istallato tutto correttamente, vi basta rilanciare `pixi run circus` per avviare la simulazione
NB: se modificate il framework (maximus) ricordate di compilarlo nuovamente con `pixi run main`
NB: se modificate il simbridge per qualche motivo, ricordate di compilarlo nuovamente con `pixi install`
NB: se modificate il Dockerfile o un qualsiasi file dentro la cartella `dockerfiles`, ricordate di compilare nuovamente il docker con `docker build -t spqr:booster dockerfiles` (se vi trovare nella root), oppure `docker build -t spqr:booster .` se vi trovate già in `dockerfiles`)

---
## Utils
- Prima che la simulazione parta, i robot si connettono al simulatore e dichiarano di essere pronti. Dopo qualche secondo, una volta che tutti i robot sono pronti, la simulazione parte. Un esempio delle stampe che dovrebbero apparire è questo:
```
Connected Robot: red_Booster-T1_0
Sending initial message to red_Booster-T1_0
Connected Robot: blue_Booster-T1_0
Sending initial message to blue_Booster-T1_0
Robot ready: red_Booster-T1_0
Robot ready: blue_Booster-T1_0
All robots are ready!
Starting simulation!
```
- Se non tutti i robot sono `ready`, la simulazione non parte. Potete capire chi è il robot problematico da quello che non dichiara di essere `ready`.
- Se un robot non parte, potete entrare nel suo docker con
    ```
    docker exec -it CIRCUS_red_Booster-T1_0_container bash
    ```
    Una volta entrati, con il comando `cat supervisord.log` potete vedere gli stati dei processi, se qualcuno è crashato, se qualcuno era crashato e ha tentato di ripartire, ...
    Se tutto è andato a buon fine, il vostro output dovrebbe essere come questo:
    ```
    2026-03-08 01:37:31,915 CRIT Supervisor is running as root.  Privileges were not dropped because no user is specified in the config file.  If you intend to run as root, you can set user=root in the config file to avoid this message.
    2026-03-08 01:37:31,929 INFO RPC interface 'supervisor' initialized
    2026-03-08 01:37:31,929 CRIT Server 'unix_http_server' running without any HTTP authentication checking
    2026-03-08 01:37:31,929 INFO supervisord started with pid 8
    2026-03-08 01:37:33,775 INFO spawned: 'booster-motion' with pid 9
    2026-03-08 01:37:33,811 INFO spawned: 'delayed-starter' with pid 10
    2026-03-08 01:37:34,813 INFO success: booster-motion entered RUNNING state, process has stayed up for > than 1 seconds (startsecs)
    2026-03-08 01:37:34,814 INFO success: delayed-starter entered RUNNING state, process has stayed up for > than 0 seconds (startsecs)
    2026-03-08 01:37:38,456 INFO spawned: 'simbridge' with pid 32
    2026-03-08 01:37:39,458 INFO success: simbridge entered RUNNING state, process has stayed up for > than 1 seconds (startsecs)
    2026-03-08 01:37:40,526 INFO spawned: 'maximus' with pid 42
    2026-03-08 01:37:41,614 INFO success: maximus entered RUNNING state, process has stayed up for > than 1 seconds (startsecs)
    2026-03-08 01:37:41,663 INFO exited: delayed-starter (exit status 0; expected)
    ```
    Il fatto che il processo `delayed-starter` termini è corretto, sono gli altri processi che non devono terminare.
- Per vedere i log di maximus, lancia il comando `cat /var/log/maximus.log` (oppure `maximus.err` per i log di errore)
- Per vedere i log di simbridge, lancia il comando `cat /var/log/simbridge.log` (oppure `simbridge.err` per i log di errore)
- Per vedere i log di booster-motion, lancia il comando `cat /var/log/booster-motion.log` (oppure `booster-motion.err` per i log di errore). Nel file `booster-motion.err`, non vi preoccupate se c'è `msgget: No such file or directory`
