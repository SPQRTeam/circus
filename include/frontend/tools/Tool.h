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

namespace spqr {

enum ToolType {
    NONE,
    PLOT,
};

class Tool : public QWidget {
        Q_OBJECT

    public:
        Tool(ToolType type, QWidget* parent = nullptr) : QWidget(parent) {
            type_ = type;
            
            setAttribute(Qt::WA_StyledBackground, true);
            setStyleSheet("QWidget { "
                            "  background-color: #1e1e1e; "
                            "  border: 1px solid #444444; "
                            "}"
                            "QWidget:hover { "
                            "  background-color: #1e1e1e; "
                            "  border: 1px solid #444444; "
                            "}"
                        );

            QVBoxLayout* layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);

            QLabel* label = new QLabel("Select a source", this);
            label->setAlignment(Qt::AlignCenter);
            label->setStyleSheet("QLabel { "
                                "  color: #888888; "
                                "  font-size: 16px; "
                                "  background-color: transparent; "
                                "  border: none; "
                                "}");
            layout->addWidget(label, 0, Qt::AlignCenter);

            setLayout(layout);
        }

        virtual ~Tool() = default;

        ToolType type() const { return type_; }

        // Virtual update method called periodically (500ms)
        virtual void update() {}

    private:
        ToolType type_;

};

}