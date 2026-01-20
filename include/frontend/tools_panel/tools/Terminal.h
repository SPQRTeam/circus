#pragma once

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QFont>
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

#include "Container.h"
#include "Pty.h"
#include "Tool.h"

namespace spqr {

class TerminalDisplay : public QTextEdit {
        Q_OBJECT

    public:
        TerminalDisplay(QWidget* parent = nullptr) : QTextEdit(parent) {
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

            setReadOnly(false);
            setUndoRedoEnabled(false);
            setLineWrapMode(QTextEdit::NoWrap);
        }

        void appendOutput(const QString& text) { processAndAppend(text); }

        void clearScreen() {
            clear();
            moveCursor(QTextCursor::Start);
        }

    signals:
        void keyInput(const QByteArray& data);

    protected:
        void keyPressEvent(QKeyEvent* event) override {
            QByteArray data;

            // Handle special keys
            switch (event->key()) {
                case Qt::Key_Return:
                case Qt::Key_Enter:
                    data = "\r";
                    break;
                case Qt::Key_Backspace:
                    data = "\x7f";  // DEL character
                    break;
                case Qt::Key_Tab:
                    data = "\t";
                    break;
                case Qt::Key_Escape:
                    data = "\x1b";
                    break;
                case Qt::Key_Up:
                    data = "\x1b[A";
                    break;
                case Qt::Key_Down:
                    data = "\x1b[B";
                    break;
                case Qt::Key_Right:
                    data = "\x1b[C";
                    break;
                case Qt::Key_Left:
                    data = "\x1b[D";
                    break;
                case Qt::Key_Home:
                    data = "\x1b[H";
                    break;
                case Qt::Key_End:
                    data = "\x1b[F";
                    break;
                case Qt::Key_PageUp:
                    data = "\x1b[5~";
                    break;
                case Qt::Key_PageDown:
                    data = "\x1b[6~";
                    break;
                case Qt::Key_Insert:
                    data = "\x1b[2~";
                    break;
                case Qt::Key_Delete:
                    data = "\x1b[3~";
                    break;
                case Qt::Key_F1:
                    data = "\x1bOP";
                    break;
                case Qt::Key_F2:
                    data = "\x1bOQ";
                    break;
                case Qt::Key_F3:
                    data = "\x1bOR";
                    break;
                case Qt::Key_F4:
                    data = "\x1bOS";
                    break;
                case Qt::Key_F5:
                    data = "\x1b[15~";
                    break;
                case Qt::Key_F6:
                    data = "\x1b[17~";
                    break;
                case Qt::Key_F7:
                    data = "\x1b[18~";
                    break;
                case Qt::Key_F8:
                    data = "\x1b[19~";
                    break;
                case Qt::Key_F9:
                    data = "\x1b[20~";
                    break;
                case Qt::Key_F10:
                    data = "\x1b[21~";
                    break;
                case Qt::Key_F11:
                    data = "\x1b[23~";
                    break;
                case Qt::Key_F12:
                    data = "\x1b[24~";
                    break;
                default:
                    // Handle Ctrl+key combinations
                    if (event->modifiers() & Qt::ControlModifier) {
                        int key = event->key();
                        if (key >= Qt::Key_A && key <= Qt::Key_Z) {
                            char ctrl = static_cast<char>(key - Qt::Key_A + 1);
                            data = QByteArray(1, ctrl);
                        } else if (key == Qt::Key_BracketLeft) {
                            data = "\x1b";  // Ctrl+[ is ESC
                        } else if (key == Qt::Key_Backslash) {
                            data = "\x1c";  // Ctrl+\ is FS
                        } else if (key == Qt::Key_BracketRight) {
                            data = "\x1d";  // Ctrl+] is GS
                        } else if (key == Qt::Key_AsciiCircum || key == Qt::Key_6) {
                            data = "\x1e";  // Ctrl+^ is RS
                        } else if (key == Qt::Key_Underscore || key == Qt::Key_Minus) {
                            data = "\x1f";  // Ctrl+_ is US
                        }
                    } else {
                        // Regular text input
                        QString text = event->text();
                        if (!text.isEmpty()) {
                            data = text.toUtf8();
                        }
                    }
                    break;
            }

            if (!data.isEmpty()) {
                emit keyInput(data);
            }

            // Don't call base class - we handle all input ourselves
            event->accept();
        }

        void contextMenuEvent(QContextMenuEvent* event) override {
            QMenu* menu = new QMenu(this);

            menu->setStyleSheet("QMenu {"
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
                                "}");

            QAction* copyAction = menu->addAction("Copy");
            copyAction->setEnabled(!textCursor().selectedText().isEmpty());
            connect(copyAction, &QAction::triggered, this, &QTextEdit::copy);

            QAction* pasteAction = menu->addAction("Paste");
            connect(pasteAction, &QAction::triggered, this, [this]() {
                // Paste by sending clipboard text as key input
                QClipboard* clipboard = QApplication::clipboard();
                QString text = clipboard->text();
                if (!text.isEmpty()) {
                    emit keyInput(text.toUtf8());
                }
            });

            menu->addSeparator();

            QAction* clearAction = menu->addAction("Clear");
            connect(clearAction, &QAction::triggered, this, &TerminalDisplay::clearScreen);

            menu->exec(event->globalPos());
            delete menu;
        }

        void resizeEvent(QResizeEvent* event) override {
            QTextEdit::resizeEvent(event);
            emit sizeChanged(calculateRows(), calculateCols());
        }

    signals:
        void sizeChanged(int rows, int cols);

    private:
        int calculateRows() const {
            QFontMetrics fm(font());
            return qMax(1, viewport()->height() / fm.lineSpacing());
        }

        int calculateCols() const {
            QFontMetrics fm(font());
            return qMax(1, viewport()->width() / fm.averageCharWidth());
        }

        void processAndAppend(const QString& input) {
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::End);

            int i = 0;
            while (i < input.size()) {
                QChar ch = input[i];

                if (ch == '\x1b' && i + 1 < input.size()) {
                    // Escape sequence
                    if (input[i + 1] == '[') {
                        // CSI sequence
                        i += 2;
                        QString params;
                        while (i < input.size() && ((input[i] >= '0' && input[i] <= '9') || input[i] == ';' ||
                                                    input[i] == '?' || input[i] == '!')) {
                            params += input[i];
                            i++;
                        }
                        if (i < input.size()) {
                            QChar cmd = input[i];
                            handleCsiSequence(cursor, cmd, params);
                            i++;
                        }
                    } else if (input[i + 1] == ']') {
                        // OSC sequence - skip until BEL or ST
                        i += 2;
                        while (i < input.size() && input[i] != '\x07') {
                            if (input[i] == '\x1b' && i + 1 < input.size() && input[i + 1] == '\\') {
                                i += 2;
                                break;
                            }
                            i++;
                        }
                        if (i < input.size() && input[i] == '\x07') i++;
                    } else if (input[i + 1] == '(' || input[i + 1] == ')') {
                        i += 3;  // Character set selection
                    } else {
                        i += 2;  // Other escape
                    }
                } else if (ch == '\x08') {
                    // Backspace - move cursor back
                    cursor.movePosition(QTextCursor::Left);
                    i++;
                } else if (ch == '\x7f') {
                    // DEL - delete character at cursor
                    cursor.deleteChar();
                    i++;
                } else if (ch == '\r') {
                    if (i + 1 < input.size() && input[i + 1] == '\n') {
                        // CRLF -> newline
                        cursor.movePosition(QTextCursor::End);
                        cursor.insertText("\n");
                        i += 2;
                    } else {
                        // CR alone - move to beginning of current line
                        cursor.movePosition(QTextCursor::StartOfBlock);
                        i++;
                    }
                } else if (ch == '\n') {
                    cursor.movePosition(QTextCursor::End);
                    cursor.insertText("\n");
                    i++;
                } else if (ch == '\x07') {
                    // Bell - skip
                    i++;
                } else if (ch.isPrint() || ch == '\t') {
                    // Regular character - insert or overwrite
                    if (!cursor.atEnd()) {
                        cursor.deleteChar();
                    }
                    cursor.insertText(QString(ch));
                    i++;
                } else {
                    // Skip other control characters
                    i++;
                }
            }

            setTextCursor(cursor);
            ensureCursorVisible();
        }

        void handleCsiSequence(QTextCursor& cursor, QChar cmd, const QString& params) {
            int n = params.isEmpty() ? 1 : params.toInt();
            if (n == 0) n = 1;

            switch (cmd.unicode()) {
                case 'A':  // Cursor up
                    cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, n);
                    break;
                case 'B':  // Cursor down
                    cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, n);
                    break;
                case 'C':  // Cursor forward
                    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, n);
                    break;
                case 'D':  // Cursor back
                    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, n);
                    break;
                case 'H':
                case 'f':  // Cursor position
                    // For simplicity, just go to start for now
                    cursor.movePosition(QTextCursor::Start);
                    break;
                case 'J':  // Erase display
                    if (params == "2" || params == "3") {
                        clearScreen();
                        cursor = textCursor();
                    }
                    break;
                case 'K':  // Erase in line
                    if (params.isEmpty() || params == "0") {
                        // Erase from cursor to end of line
                        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                        cursor.removeSelectedText();
                    } else if (params == "1") {
                        // Erase from start of line to cursor
                        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
                        cursor.removeSelectedText();
                    } else if (params == "2") {
                        // Erase entire line
                        cursor.movePosition(QTextCursor::StartOfBlock);
                        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                        cursor.removeSelectedText();
                    }
                    break;
                case 'P':  // Delete characters
                    for (int j = 0; j < n; j++) {
                        cursor.deleteChar();
                    }
                    break;
                case 'm':  // SGR (colors) - ignore
                case 'h':  // Set mode - ignore
                case 'l':  // Reset mode - ignore
                case 'r':  // Set scrolling region - ignore
                default:
                    break;
            }
        }
};

class Terminal : public Tool {
        Q_OBJECT

    public:
        Terminal(std::shared_ptr<Container> container, QWidget* parent = nullptr)
            : Tool(ToolType::TERMINAL, parent), container_(container), pty_(nullptr) {
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

            // Terminal display
            display_ = new TerminalDisplay(this);
            layout->addWidget(display_, 1);

            setLayout(layout);

            // Connect display signals
            connect(display_, &TerminalDisplay::keyInput, this, &Terminal::onKeyInput);
            connect(display_, &TerminalDisplay::sizeChanged, this, &Terminal::onSizeChanged);

            // Initialize PTY shell
            initializeShell();
        }

        ~Terminal() override {
            if (pty_) {
                pty_->stop();
            }
        }

        void update() override {
            // Output is handled by dataReceived signal
        }

    private slots:
        void onKeyInput(const QByteArray& data) {
            if (pty_ && pty_->isRunning()) {
                pty_->write(data);
            }
        }

        void onSizeChanged(int rows, int cols) {
            if (pty_ && pty_->isRunning()) {
                pty_->resize(rows, cols);
            }
        }

        void onDataReceived(const QByteArray& data) {
            QString text = QString::fromUtf8(data);
            display_->appendOutput(text);
        }

        void onPtyError(const QString& message) { display_->appendOutput("Error: " + message + "\n"); }

        void onPtyFinished(int exitCode) {
            display_->appendOutput("\n[Shell exited with code " + QString::number(exitCode) + "]\n");
        }

    private:
        void initializeShell() {
            if (!container_) {
                display_->appendOutput("Error: No container available\n");
                return;
            }

            QString containerId = QString::fromStdString(container_->getId());

            pty_ = new Pty(this);

            connect(pty_, &Pty::dataReceived, this, &Terminal::onDataReceived);
            connect(pty_, &Pty::error, this, &Terminal::onPtyError);
            connect(pty_, &Pty::finished, this, &Terminal::onPtyFinished);

            display_->appendOutput("Connecting to container " + containerId.left(12) + "...\n\n");

            if (!pty_->start(containerId)) {
                display_->appendOutput("Failed to start PTY shell\n");
            }
        }

        std::shared_ptr<Container> container_;
        TerminalDisplay* display_;
        Pty* pty_;
};

}  // namespace spqr
