#pragma once

#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "GameController.h"

namespace spqr {

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
            outputArea_->setText("Game Controller Console ready.\n");
            layout->addWidget(outputArea_, 1);

            // Input area (command input)
            inputArea_ = new QTextEdit(this);
            inputArea_->setMaximumHeight(60);
            inputArea_->setStyleSheet("QTextEdit { "
                                      "  background-color: #1a1a1a; "
                                      "  color: #ffffff; "
                                      "  border: 1px solid #006778; "
                                      "  font-family: monospace; "
                                      "  font-size: 11px; "
                                      "}");
            inputArea_->setPlaceholderText("Enter command...");
            layout->addWidget(inputArea_);

            setLayout(layout);

            // Connect input signals
            connect(inputArea_, &QTextEdit::textChanged, this, &ConsoleWidget::onInputChanged);
        }

        virtual ~ConsoleWidget() = default;

    private slots:
        void onInputChanged() {
            QString text = inputArea_->toPlainText();
            if (text.contains('\n')) {
                // Extract command (remove newline)
                QString command = text.trimmed();
                inputArea_->clear();

                // Process command
                if (!command.isEmpty()) {
                    sendCommand(command);
                }
            }
        }

    private:
        void sendCommand(const QString& command) {
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
        QTextEdit* inputArea_;
};

}  // namespace spqr
