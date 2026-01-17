#pragma once

#include <QHBoxLayout>
#include <QSizePolicy>
#include <QWidget>

#include "frontend/game_controller_panel_header/GameControllerPanelHeader.h"

namespace spqr {

class GameControllerPanelHeaderContainer : public QWidget {
        Q_OBJECT

    public:
        GameControllerPanelHeaderContainer(QWidget* parent = nullptr) : QWidget(parent) {
            // Main horizontal layout
            QHBoxLayout* mainLayout = new QHBoxLayout(this);
            mainLayout->setContentsMargins(5, 5, 5, 0);
            mainLayout->setSpacing(0);

            // Create the header
            header_ = new GameControllerPanelHeader(this);
            mainLayout->addWidget(header_);

            setLayout(mainLayout);

            // Set size policy to expand horizontally but fixed height
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }

        GameControllerPanelHeader* getHeader() const {
            return header_;
        }

    private:
        GameControllerPanelHeader* header_;
};

}  // namespace spqr
