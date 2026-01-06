#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QMouseEvent>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

namespace spqr {

class GridCell : public QWidget {
        Q_OBJECT

    public:
        GridCell(QWidget* parent = nullptr) : QWidget(parent) {
            setStyleSheet("GridCell { "
                         "  background-color: #2a2a2a; "
                         "  border: 1px solid #444444; "
                         "}");
            setMinimumSize(100, 100);
        }
};

class ToolsPanelGrid : public QWidget {
        Q_OBJECT

    public:
        ToolsPanelGrid(QWidget* parent = nullptr) : QWidget(parent) {
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
                GridCell* cell = new GridCell(gridContainer_);
                layout->addWidget(cell);
            } else if (numRows_ == 1) {
                // Single row: horizontal splitter
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                QSplitter* splitter = new QSplitter(Qt::Horizontal, gridContainer_);
                splitter->setHandleWidth(2);
                splitter->setStyleSheet("QSplitter::handle { background-color: #555555; }");

                for (int col = 0; col < numCols_; col++) {
                    GridCell* cell = new GridCell(splitter);
                    splitter->addWidget(cell);
                }

                layout->addWidget(splitter);
            } else if (numCols_ == 1) {
                // Single column: vertical splitter
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                QSplitter* splitter = new QSplitter(Qt::Vertical, gridContainer_);
                splitter->setHandleWidth(2);
                splitter->setStyleSheet("QSplitter::handle { background-color: #555555; }");

                for (int row = 0; row < numRows_; row++) {
                    GridCell* cell = new GridCell(splitter);
                    splitter->addWidget(cell);
                }

                layout->addWidget(splitter);
            } else {
                // Multiple rows and columns: nested splitters
                QVBoxLayout* layout = new QVBoxLayout(gridContainer_);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->setSpacing(0);

                QSplitter* verticalSplitter = new QSplitter(Qt::Vertical, gridContainer_);
                verticalSplitter->setHandleWidth(2);
                verticalSplitter->setStyleSheet("QSplitter::handle { background-color: #555555; }");

                for (int row = 0; row < numRows_; row++) {
                    QSplitter* horizontalSplitter = new QSplitter(Qt::Horizontal, verticalSplitter);
                    horizontalSplitter->setHandleWidth(2);
                    horizontalSplitter->setStyleSheet("QSplitter::handle { background-color: #555555; }");

                    for (int col = 0; col < numCols_; col++) {
                        GridCell* cell = new GridCell(horizontalSplitter);
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
};

}  // namespace spqr
