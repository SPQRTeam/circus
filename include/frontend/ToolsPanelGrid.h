#pragma once

#include <qobject.h>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QListView>
#include <QMouseEvent>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "tools/Tool.h"

namespace spqr {

class GridCell : public QWidget {
        Q_OBJECT

    public:
        GridCell(QStringList streams, QWidget* parent = nullptr) : QWidget(parent) {
            setAttribute(Qt::WA_StyledBackground, true);
            setStyleSheet("QWidget { "
                            "  background-color: #2a2a2a; "
                            "  border: 1px solid #444444; "
                            "  border-radius: 3px;"
                            "}"
                            "QWidget:hover { "
                            "  background-color: #3a3a3a; "
                            "  border: 2px solid #1e667e; "
                            "}");
            setMinimumSize(100, 100);

            QVBoxLayout* layout = new QVBoxLayout(this);
            layout->setContentsMargins(6, 6, 6, 6);
            layout->setSpacing(6);

            QComboBox* combo = new QComboBox(this);
            combo->setMaxVisibleItems(6);
            combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
            combo->setEditable(true);

            QListView* popupView = new QListView(combo);
            popupView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            popupView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            popupView->setFrameShape(QFrame::NoFrame);
            popupView->setSpacing(0);
            popupView->setUniformItemSizes(true);
            popupView->setContentsMargins(0, 0, 0, 0);
            popupView->setStyleSheet("QListView { "
                                     "  background-color: #2a2a2a; "
                                     "  color: white; "
                                     "  border: 1px solid #1e667e; "
                                     "  padding: 0; "
                                     "  margin: 0; "
                                     "  outline: 0; "
                                     "} "
                                     "QListView::item { "
                                     "  padding: 6px 10px; "
                                     "  margin: 0; "
                                     "} "
                                     "QListView::item:selected { "
                                     "  background-color: #1e667e; "
                                     "  color: white; "
                                     "}");
            combo->setView(popupView);

            combo->setStyleSheet("QComboBox { "
                                "  background-color: #444444; "
                                "  color: white; "
                                "  border: 1px solid #666666; "
                                "  border-radius: 3px; "
                                "  padding: 5px 10px 5px 10px; "
                                "  font-size: 13px; "
                                "} "
                                "QComboBox:hover { "
                                "  background-color: #595959; "
                                "  border: 1px solid #1e667e; "
                                "} "
                            );

            combo->addItems(streams);
            selectedItem_ = combo->currentText();
            connect(combo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
                selectedItem_ = text;
            });
            layout->addWidget(combo, 0, Qt::AlignTop);

            // Add the tool widget that fills the remaining space
            tool_ = new Tool(this);
            layout->addWidget(tool_, 1);
        }

        QString selectedItem() const { return selectedItem_; }

    private:
        QString selectedItem_;
        Tool* tool_;
};

class ToolsPanelGrid : public QWidget {
        Q_OBJECT

    public:
        ToolsPanelGrid(QStringList streams, QWidget* parent = nullptr) : QWidget(parent) {
            streams_ = streams;

            QVBoxLayout* mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(0);

            // Create the grid container
            gridContainer_ = new QWidget(this);
            mainLayout->addWidget(gridContainer_);

            // Initialize with 1x1 grid
            numRows_ = 1;
            numCols_ = 1;
            rebuildGrid();

            setLayout(mainLayout);
        }

        int getNumRows() const { return numRows_; }
        int getNumCols() const { return numCols_; }

    public slots:
        void addRow() {
            numRows_++;
            rebuildGrid();
            emit gridSizeChanged(numRows_, numCols_);
        }

        void removeRow() {
            if (numRows_ > 1) {
                numRows_--;
                rebuildGrid();
                emit gridSizeChanged(numRows_, numCols_);
            }
        }

        void addColumn() {
            numCols_++;
            rebuildGrid();
            emit gridSizeChanged(numRows_, numCols_);
        }

        void removeColumn() {
            if (numCols_ > 1) {
                numCols_--;
                rebuildGrid();
                emit gridSizeChanged(numRows_, numCols_);
            }
        }

    signals:
        void gridSizeChanged(int rows, int cols);

    private:
        void rebuildGrid() {
            // Clear existing layout
            if (gridContainer_->layout()) {
                QLayout* oldLayout = gridContainer_->layout();
                QLayoutItem* item;
                while ((item = oldLayout->takeAt(0)) != nullptr) {
                    if (item->widget()) {
                        delete item->widget();
                    }
                    delete item;
                }
                delete oldLayout;
            }

            // Build nested splitters for resizable grid
            if (numRows_ == 1 && numCols_ == 1) {
                // Simple case: single cell
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);
                GridCell* cell = new GridCell(streams_, gridContainer_);
                layout->addWidget(cell);
            } else if (numRows_ == 1) {
                // Single row: horizontal splitter
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                QSplitter* splitter = new QSplitter(Qt::Horizontal, gridContainer_);
                splitter->setHandleWidth(6);
                splitter->setStyleSheet("QSplitter::handle { background-color: #1a1a1a; }");

                for (int col = 0; col < numCols_; col++) {
                    GridCell* cell = new GridCell(streams_, splitter);
                    splitter->addWidget(cell);
                }

                layout->addWidget(splitter);
            } else if (numCols_ == 1) {
                // Single column: vertical splitter
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                QSplitter* splitter = new QSplitter(Qt::Vertical, gridContainer_);
                splitter->setHandleWidth(6);
                splitter->setStyleSheet("QSplitter::handle { background-color: #1a1a1a; }");

                for (int row = 0; row < numRows_; row++) {
                    GridCell* cell = new GridCell(streams_, splitter);
                    splitter->addWidget(cell);
                }

                layout->addWidget(splitter);
            } else {
                // Multiple rows and columns: nested splitters
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                QSplitter* verticalSplitter = new QSplitter(Qt::Vertical, gridContainer_);
                verticalSplitter->setHandleWidth(6);
                verticalSplitter->setStyleSheet("QSplitter::handle { background-color: #1a1a1a; }");

                for (int row = 0; row < numRows_; row++) {
                    QSplitter* horizontalSplitter = new QSplitter(Qt::Horizontal, verticalSplitter);
                    horizontalSplitter->setHandleWidth(6);
                    horizontalSplitter->setStyleSheet("QSplitter::handle { background-color: #1a1a1a; }");

                    for (int col = 0; col < numCols_; col++) {
                        GridCell* cell = new GridCell(streams_, horizontalSplitter);
                        horizontalSplitter->addWidget(cell);
                    }

                    verticalSplitter->addWidget(horizontalSplitter);
                }

                layout->addWidget(verticalSplitter);
            }
        }

        QWidget* gridContainer_;
        int numRows_;
        int numCols_;
        QStringList streams_;
};

}  // namespace spqr
