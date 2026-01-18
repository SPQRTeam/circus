#pragma once

#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "GameController.h"

namespace spqr {

class CommandLineEdit : public QLineEdit {
        Q_OBJECT

    public:
        CommandLineEdit(QWidget* parent = nullptr) : QLineEdit(parent) {}

    signals:
        void upPressed();
        void downPressed();

    protected:
        void keyPressEvent(QKeyEvent* event) override {
            if (event->key() == Qt::Key_Up) {
                emit upPressed();
                event->accept();
            } else if (event->key() == Qt::Key_Down) {
                emit downPressed();
                event->accept();
            } else {
                QLineEdit::keyPressEvent(event);
            }
        }
};

class ConsoleWidget : public QWidget {
        Q_OBJECT

    public:
        ConsoleWidget(QWidget* parent = nullptr) : QWidget(parent) {
            setAttribute(Qt::WA_StyledBackground, true);
            setStyleSheet("QWidget { "
                          "  background-color: #0a0a0a; "
                          "  border: 1px solid #444444; "
                          "}");

            QVBoxLayout* layout = new QVBoxLayout(this);
            layout->setContentsMargins(5, 5, 5, 5);
            layout->setSpacing(5);

            // Header label
            QLabel* header = new QLabel("Game Controller Console", this);
            header->setStyleSheet("QLabel { "
                                  "  color: #00a0b0; "
                                  "  font-size: 14px; "
                                  "  font-weight: bold; "
                                  "  background-color: transparent; "
                                  "  border: none; "
                                  "}");
            layout->addWidget(header);

            // Output area (read-only console output)
            outputArea_ = new QTextEdit(this);
            outputArea_->setReadOnly(true);
            outputArea_->setStyleSheet("QTextEdit { "
                                       "  background-color: #1a1a1a; "
                                       "  color: #cccccc; "
                                       "  border: 1px solid #444444; "
                                       "  font-family: monospace; "
                                       "  font-size: 11px; "
                                       "}");
            outputArea_->setText("Game Controller Console ready.\nType 'help' for a list of commands.\n");
            layout->addWidget(outputArea_, 1);

            // Input area (command input)
            inputArea_ = new CommandLineEdit(this);
            inputArea_->setStyleSheet("QLineEdit { "
                                      "  background-color: #1a1a1a; "
                                      "  color: #ffffff; "
                                      "  border: 1px solid #006778; "
                                      "  font-family: monospace; "
                                      "  font-size: 11px; "
                                      "  padding: 5px; "
                                      "}");
            inputArea_->setPlaceholderText("Enter command...");
            layout->addWidget(inputArea_);

            setLayout(layout);

            // Connect input signals
            connect(inputArea_, &QLineEdit::returnPressed, this, &ConsoleWidget::onReturnPressed);
            connect(inputArea_, &CommandLineEdit::upPressed, this, &ConsoleWidget::onUpPressed);
            connect(inputArea_, &CommandLineEdit::downPressed, this, &ConsoleWidget::onDownPressed);
        }

        virtual ~ConsoleWidget() = default;

    private slots:
        void onReturnPressed() {
            QString command = inputArea_->text().trimmed();
            inputArea_->clear();

            if (!command.isEmpty()) {
                sendCommand(command);
            }
        }

        void onUpPressed() {
            if (commandHistory_.isEmpty()) return;

            if (historyIndex_ == -1) {
                // Save current input before navigating history
                currentInput_ = inputArea_->text();
                historyIndex_ = commandHistory_.size() - 1;
            } else if (historyIndex_ > 0) {
                historyIndex_--;
            }

            inputArea_->setText(commandHistory_[historyIndex_]);
        }

        void onDownPressed() {
            if (commandHistory_.isEmpty() || historyIndex_ == -1) return;

            if (historyIndex_ < commandHistory_.size() - 1) {
                historyIndex_++;
                inputArea_->setText(commandHistory_[historyIndex_]);
            } else {
                // Return to current input
                historyIndex_ = -1;
                inputArea_->setText(currentInput_);
            }
        }

    private:
        void sendCommand(const QString& command) {
            // Add to history
            if (commandHistory_.isEmpty() || commandHistory_.last() != command) {
                commandHistory_.append(command);
            }
            historyIndex_ = -1;
            currentInput_.clear();

            // Log the command to output
            outputArea_->append(QString("> %1").arg(command));

            // Process game controller commands
            if (command == "clear") {
                outputArea_->clear();
                outputArea_->append("Game Controller Console ready.");
                return;
            } else if (command == "help") {
                outputArea_->append(QString::fromStdString(helpMessage()));
                return;
            }

            std::string cmdStr = command.toStdString();
            if (GameController::instance().isCommandValid(cmdStr)) {
                std::string res = GameController::instance().handleCommand(cmdStr);
                outputArea_->append(QString("%1").arg(QString::fromStdString(res)));
            } else {
                outputArea_->append(QString("Unknown command: %1").arg(command));
                outputArea_->append(QString::fromStdString(helpMessage()));
            }
        }

        std::string helpMessage() {
            std::string helpMsg = "Available commands:\n"
                                  "  clear   - Clear the console output\n"
                                  "  help    - Show this help message\n";

            auto commands = GameController::instance().availableCommands();
            for (const auto& [cmd, desc] : commands) {
                helpMsg += "  " + cmd + " - " + desc + "\n";
            }

            return helpMsg;
        }

        QTextEdit* outputArea_;
        CommandLineEdit* inputArea_;
        QStringList commandHistory_;
        int historyIndex_ = -1;
        QString currentInput_;
};

}  // namespace spqr
