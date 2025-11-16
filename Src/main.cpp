
#include <linux/prctl.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <QSurfaceFormat>
#include <csignal>
#include <iostream>

#include "AppWindow.h"
#include "CircusApplication.h"
#include "Constants.h"

int runCircusApp(int argc, char** argv) {
    spqr::CircusApplication app(argc, argv);
    app.setApplicationName(spqr::appName);

    spqr::AppWindow window(argc, argv);
    window.show();

    return app.exec();
}

int main(int argc, char** argv) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
    }

    if (pid == 0) {
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        return runCircusApp(argc, argv);
    } else {
        // ignore all signal sent to him except of the uncatchable ones
        sigset_t mask;
        sigfillset(&mask);
        sigprocmask(SIG_SETMASK, &mask, NULL);

        int status;
        waitpid(pid, &status, 0);

        FILE* dockerList = popen("docker ps -aq", "r");
        char buffer[128];

        if (fgets(buffer, sizeof(buffer), dockerList) != nullptr) {
            std::cout << "Dockers still running: forcing their closure.\n";
        }

        pclose(dockerList);
        system("docker rm -f $(docker ps -aq) > /dev/null 2>&1");
        return WEXITSTATUS(status);
    }
}
