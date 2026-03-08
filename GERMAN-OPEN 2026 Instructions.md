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
