#pragma once

#include <qobject.h>

#include <QContextMenuEvent>
#include <QFont>
#include <QKeyEvent>
#include <QMenu>
#include <QProcess>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <deque>
#include <memory>

#include "Container.h"
#include "Tool.h"

namespace spqr {

class TerminalDisplay : public QTextEdit {
        Q_OBJECT

    public:
        TerminalDisplay(QWidget* parent = nullptr) : QTextEdit(parent), inputStartPos_(0) {
            setStyleSheet("QTextEdit { "
                          "  background-color: #0c0c0c; "
                          "  color: #cccccc; "
                          "  border: none; "
                          "  font-family: 'Courier New', monospace; "
                          "  font-size: 12px; "
                          "}");

            // Set monospace font
            QFont font("Courier New", 10);
            font.setStyleHint(QFont::Monospace);
            setFont(font);

            // Make it editable for typing commands
            setReadOnly(false);
            setUndoRedoEnabled(false);
        }

        void appendOutput(const QString& text) {
            moveCursor(QTextCursor::End);
            insertPlainText(text);
            moveCursor(QTextCursor::End);
            inputStartPos_ = textCursor().position();
            ensureCursorVisible();
        }

        QString getCurrentCommand() {
            QTextCursor cursor = textCursor();
            cursor.setPosition(inputStartPos_);
            cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            return cursor.selectedText();
        }

        void clearCurrentCommand() {
            QTextCursor cursor = textCursor();
            cursor.setPosition(inputStartPos_);
            cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }

    signals:
        void commandEntered(const QString& command);
        void upPressed();
        void downPressed();

    protected:
        void keyPressEvent(QKeyEvent* event) override {
            // Prevent editing before input start position
            if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Left) {
                if (textCursor().position() <= inputStartPos_) {
                    event->accept();
                    return;
                }
            }

            if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
                QString command = getCurrentCommand();
                appendOutput("\n");
                emit commandEntered(command);
                event->accept();
            } else if (event->key() == Qt::Key_Up) {
                emit upPressed();
                event->accept();
            } else if (event->key() == Qt::Key_Down) {
                emit downPressed();
                event->accept();
            } else {
                QTextEdit::keyPressEvent(event);
            }
        }

        void contextMenuEvent(QContextMenuEvent* event) override {
            QMenu* menu = new QMenu(this);

            // Style the menu to match the terminal theme
            menu->setStyleSheet(
                "QMenu {"
                "  background-color: #1e1e1e;"
                "  color: #cccccc;"
                "  border: 1px solid #444444;"
                "  padding: 4px;"
                "}"
                "QMenu::item {"
                "  padding: 6px 20px;"
                "  border-radius: 3px;"
                "}"
                "QMenu::item:selected {"
                "  background-color: #2d2d2d;"
                "}"
                "QMenu::item:disabled {"
                "  color: #666666;"
                "}"
            );

            // Add Copy action
            QAction* copyAction = menu->addAction("Copy");
            copyAction->setEnabled(!textCursor().selectedText().isEmpty());
            connect(copyAction, &QAction::triggered, this, &QTextEdit::copy);

            // Add Paste action
            QAction* pasteAction = menu->addAction("Paste");
            connect(pasteAction, &QAction::triggered, this, &QTextEdit::paste);

            menu->exec(event->globalPos());
            delete menu;
        }

    private:
        int inputStartPos_;
};


class Terminal : public Tool {
        Q_OBJECT

    public:
        Terminal(std::shared_ptr<Container> container, QWidget* parent = nullptr)
            : Tool(ToolType::TERMINAL, parent), container_(container), historyIndex_(-1) {
            // Clear the default "Select a source" label from base Tool class
            QLayout* oldLayout = layout();
            if (oldLayout) {
                QLayoutItem* item;
                while ((item = oldLayout->takeAt(0)) != nullptr) {
                    if (item->widget()) {
                        delete item->widget();
                    }
                    delete item;
                }
                delete oldLayout;
            }

            // Create new layout
            QVBoxLayout* layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);

            // Terminal display (unified input/output)
            display_ = new TerminalDisplay(this);
            layout->addWidget(display_, 1);

            setLayout(layout);

            // Connect display signals
            connect(display_, &TerminalDisplay::commandEntered, this, &Terminal::onCommandEntered);
            connect(display_, &TerminalDisplay::upPressed, this, &Terminal::onUpPressed);
            connect(display_, &TerminalDisplay::downPressed, this, &Terminal::onDownPressed);

            // Initialize shell
            initializeShell();
        }

        ~Terminal() override {
            if (process_ && process_->state() != QProcess::NotRunning) {
                process_->terminate();
                process_->waitForFinished(1000);
            }
        }

        void update() override {
            // Output is handled by readyReadStandardOutput signal
        }

    private slots:
        void onCommandEntered(const QString& command) {
            // Add to history
            if (!command.isEmpty()) {
                history_.push_back(command);
                historyIndex_ = history_.size();
            }

            // Send command to process
            if (process_ && process_->state() == QProcess::Running) {
                // Send the actual command
                process_->write((command + "\n").toUtf8());
                // After the command, get pwd and print a marker
                process_->write("echo \"__PROMPT__$(whoami)@$(hostname):$(pwd)$ \"\n");
                process_->waitForBytesWritten();
            } else {
                display_->appendOutput("Error: Shell not running\n");
            }
        }

        void onUpPressed() {
            // Navigate history up
            if (historyIndex_ > 0) {
                historyIndex_--;
                display_->clearCurrentCommand();
                display_->appendOutput(history_[historyIndex_]);
            }
        }

        void onDownPressed() {
            // Navigate history down
            if (historyIndex_ < static_cast<int>(history_.size()) - 1) {
                historyIndex_++;
                display_->clearCurrentCommand();
                display_->appendOutput(history_[historyIndex_]);
            } else if (historyIndex_ == static_cast<int>(history_.size()) - 1) {
                historyIndex_++;
                display_->clearCurrentCommand();
            }
        }

        void onProcessError(QProcess::ProcessError error) {
            QString errorMsg;
            switch (error) {
                case QProcess::FailedToStart:
                    errorMsg = "Failed to start shell process\n";
                    break;
                case QProcess::Crashed:
                    errorMsg = "Shell process crashed\n";
                    break;
                default:
                    errorMsg = "Shell process error\n";
                    break;
            }
            display_->appendOutput(errorMsg);
        }

        void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitStatus == QProcess::CrashExit) {
                display_->appendOutput("\nShell crashed\n");
            } else {
                display_->appendOutput("\nShell exited with code " + QString::number(exitCode) + "\n");
            }
        }

    private:
        void initializeShell() {
            if (!container_) {
                display_->appendOutput("Error: No container available\n");
                return;
            }

            QString containerId = QString::fromStdString(container_->getId());

            // Use docker exec to start an interactive bash shell
            process_ = new QProcess(this);

            // Set process channel mode to merge stdout and stderr
            process_->setProcessChannelMode(QProcess::MergedChannels);

            // Connect signals
            connect(process_, &QProcess::errorOccurred, this, &Terminal::onProcessError);
            connect(process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Terminal::onProcessFinished);
            connect(process_, &QProcess::readyReadStandardOutput, this, &Terminal::onReadyRead);

            // Start docker exec with interactive bash and proper environment
            QStringList args;
            args << "exec" << "-i"
                 << "-e" << "TERM=xterm-256color"
                 << containerId
                 << "/bin/bash";

            display_->appendOutput("Starting shell in container " + containerId.left(12) + " ...\n");

            process_->start("docker", args);

            if (!process_->waitForStarted(3000)) {
                display_->appendOutput("Failed to start shell\n");
            } else {
                display_->appendOutput("Shell started.\n\n");

                // Set TERM and display initial prompt
                process_->write("export TERM=xterm-256color\n");
                process_->write("echo \"__PROMPT__$(whoami)@$(hostname):$(pwd)$ \"\n");
                process_->waitForBytesWritten();
            }
        }

        void onReadyRead() {
            if (process_ && process_->state() == QProcess::Running) {
                QString output = QString::fromUtf8(process_->readAllStandardOutput());
                if (!output.isEmpty()) {
                    // Check for clear screen escape sequence
                    if (output.contains("\033[H\033[2J") || output.contains("\033[3J")) {
                        display_->clear();
                        // Remove the escape sequences
                        output.remove("\033[H");
                        output.remove("\033[2J");
                        output.remove("\033[3J");
                        output.remove("[H");
                        output.remove("[2J");
                        output.remove("[3J");
                    }

                    // Check if output contains our prompt marker
                    if (output.contains("__PROMPT__")) {
                        // Split by the marker
                        QStringList parts = output.split("__PROMPT__");

                        // Display everything before the marker (command output)
                        if (!parts[0].isEmpty()) {
                            display_->appendOutput(parts[0]);
                        }

                        // The prompt is after the marker, extract it
                        if (parts.size() > 1) {
                            QString promptLine = parts[1];
                            // Find the end of the prompt line (ends with "$ ")
                            int promptEnd = promptLine.indexOf("$ ");
                            if (promptEnd >= 0) {
                                QString prompt = promptLine.left(promptEnd + 2);  // Include "$ "
                                display_->appendOutput(prompt);

                                // Display any remaining output after the prompt
                                QString remaining = promptLine.mid(promptEnd + 2);
                                if (!remaining.trimmed().isEmpty()) {
                                    display_->appendOutput(remaining);
                                }
                            }
                        }
                    } else if (!output.trimmed().isEmpty()) {
                        display_->appendOutput(output);
                    }
                }
            }
        }

        std::shared_ptr<Container> container_;
        TerminalDisplay* display_;
        QProcess* process_;
        std::deque<QString> history_;
        int historyIndex_;
};

}  // namespace spqr
