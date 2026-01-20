#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <QString>
#include <QTimer>

#include <errno.h>
#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <cstring>
#include <functional>

namespace spqr {

class Pty : public QObject {
        Q_OBJECT

    public:
        explicit Pty(QObject* parent = nullptr) : QObject(parent), masterFd_(-1), childPid_(-1), notifier_(nullptr) {}

        ~Pty() override { stop(); }

        bool start(const QString& containerId) {
            struct winsize ws;
            ws.ws_row = 24;
            ws.ws_col = 80;
            ws.ws_xpixel = 0;
            ws.ws_ypixel = 0;

            childPid_ = forkpty(&masterFd_, nullptr, nullptr, &ws);

            if (childPid_ < 0) {
                emit error("Failed to forkpty: " + QString::fromStdString(std::strerror(errno)));
                return false;
            }

            if (childPid_ == 0) {
                // Child process - exec docker
                setenv("TERM", "xterm-256color", 1);

                execlp("docker", "docker", "exec", "-it", containerId.toStdString().c_str(), "/bin/bash", nullptr);

                // If exec fails
                _exit(1);
            }

            // Parent process
            // Set master fd to non-blocking
            int flags = fcntl(masterFd_, F_GETFL, 0);
            fcntl(masterFd_, F_SETFL, flags | O_NONBLOCK);

            // Create socket notifier to watch for data
            notifier_ = new QSocketNotifier(masterFd_, QSocketNotifier::Read, this);
            connect(notifier_, &QSocketNotifier::activated, this, &Pty::onReadyRead);

            return true;
        }

        void stop() {
            if (notifier_) {
                notifier_->setEnabled(false);
                delete notifier_;
                notifier_ = nullptr;
            }

            if (masterFd_ >= 0) {
                close(masterFd_);
                masterFd_ = -1;
            }

            if (childPid_ > 0) {
                kill(childPid_, SIGTERM);
                int status;
                waitpid(childPid_, &status, WNOHANG);
                childPid_ = -1;
            }
        }

        void write(const QByteArray& data) {
            if (masterFd_ >= 0) {
                ::write(masterFd_, data.constData(), data.size());
            }
        }

        void resize(int rows, int cols) {
            if (masterFd_ >= 0) {
                struct winsize ws;
                ws.ws_row = rows;
                ws.ws_col = cols;
                ws.ws_xpixel = 0;
                ws.ws_ypixel = 0;
                ioctl(masterFd_, TIOCSWINSZ, &ws);
            }
        }

        bool isRunning() const { return childPid_ > 0 && masterFd_ >= 0; }

    signals:
        void dataReceived(const QByteArray& data);
        void finished(int exitCode);
        void error(const QString& message);

    private slots:
        void onReadyRead() {
            if (masterFd_ < 0) return;

            char buffer[4096];
            ssize_t bytesRead = ::read(masterFd_, buffer, sizeof(buffer));

            if (bytesRead > 0) {
                emit dataReceived(QByteArray(buffer, bytesRead));
            } else if (bytesRead == 0 || (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                // EOF or error - process ended
                if (childPid_ > 0) {
                    int status;
                    waitpid(childPid_, &status, WNOHANG);
                    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                    childPid_ = -1;
                    emit finished(exitCode);
                }
            }
        }

    private:
        int masterFd_;
        pid_t childPid_;
        QSocketNotifier* notifier_;
};

}  // namespace spqr
