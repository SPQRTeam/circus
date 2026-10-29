#pragma once

#include <GameController.h>
#include <RoboCupGameControlData.h>

#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>
#include <array>
#include <cstdlib>
#include <functional>
#include <optional>
#include <vector>

#include "GameControllerActions.h"
#include "GameControllerBridge.h"
#include "frontend/tools_panel/tools/Tool.h"

// Operator interface of the official GameController, ported from the React frontend in
// external/GameController/frontend/src/components/main. The layout, labels, colors and the
// rules for showing and enabling the controls follow that implementation.

namespace spqr {

namespace gcui {

// Jersey colors as named by the frontend, indexed by the TEAM_* values of the control message.
inline QString colorName(uint8_t colour) {
    static const char* names[] = {"blue", "red", "yellow", "black", "white", "green", "orange", "purple", "brown", "gray"};
    return colour < 10 ? names[colour] : "gray";
}

// Tailwind colors of the team names (textClasses in TeamPanel.jsx).
inline QString textColor(const QString& color) {
    static const QMap<QString, QString> colors = {
        {"red", "#dc2626"},   {"blue", "#2563eb"},   {"yellow", "#facc15"}, {"black", "#000000"}, {"white", "#000000"},
        {"green", "#16a34a"}, {"orange", "#fb923c"}, {"purple", "#9333ea"}, {"brown", "#92400e"}, {"gray", "#4b5563"},
    };
    return colors.value(color, "#000000");
}

// Tailwind colors of the player buttons (bgClasses in PlayerButton.jsx).
inline QString backgroundColor(const QString& color) {
    static const QMap<QString, QString> colors = {
        {"red", "#fee2e2"},   {"blue", "#dbeafe"},   {"yellow", "#fef9c3"}, {"black", "#ffffff"}, {"white", "#ffffff"},
        {"green", "#dcfce7"}, {"orange", "#ffedd5"}, {"purple", "#f3e8ff"}, {"brown", "#fef3c7"}, {"gray", "#e5e7eb"},
    };
    return colors.value(color, "#ffffff");
}

inline QString capitalized(const QString& text) {
    return text.isEmpty() ? text : text.left(1).toUpper() + text.mid(1);
}

inline QString formatMMSS(int rawSeconds) {
    QString sign = rawSeconds < 0 ? "-" : "";
    int seconds = std::abs(rawSeconds);
    return sign + QString("%1:%2").arg(seconds / 60, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0'));
}

// The flash animations of the frontend switch to yellow during the second half of every second.
inline bool flashOn() {
    return QTime::currentTime().msec() >= 500;
}

// penaltyDescriptions in PlayerButton.jsx.
inline QString penaltyDescription(uint8_t penalty) {
    switch (penalty) {
        case PENALTY_ILLEGAL_POSITIONING:
            return "Illegal Positioning";
        case PENALTY_MOTION_IN_SET:
            return "Motion in Set";
        case PENALTY_MOTION_IN_STOP:
            return "Motion in Stop";
        case PENALTY_LOCAL_GAME_STUCK:
            return "Local Game Stuck";
        case PENALTY_INCAPABLE_ROBOT:
            return "Incapable Robot";
        case PENALTY_PICK_UP:
            return "Picked Up";
        case PENALTY_BALL_HOLDING:
            return "Ball Holding";
        case PENALTY_LEAVING_THE_FIELD:
            return "Leaving the Field";
        case PENALTY_PLAYING_WITH_ARMS_HANDS:
            return "Arms / Hands";
        case PENALTY_PUSHING:
            return "Pushing";
        case PENALTY_CAUTIONED:
            return "Cautioned";
        case PENALTY_SENT_OFF:
            return "Sent Off";
        case PENALTY_SUBSTITUTE:
            return "Substitute";
        default:
            return "No Penalty";
    }
}

// The sizes below are those of the React frontend (Tailwind px). In the simulator the panel shares
// the window with the 3D view, so it is drawn more compact by scaling fonts and dimensions.
constexpr double kFontScale = 0.8;
constexpr double kSizeScale = 0.6;

inline int font(int px) {
    return static_cast<int>(px * kFontScale + 0.5);
}

inline int dp(int px) {
    return static_cast<int>(px * kSizeScale + 0.5);
}

inline QString labelStyle(const QString& color, int fontPx, int weight = 400) {
    return QString("QLabel { color: %1; background: transparent; border: none; font-size: %2px; font-weight: %3; }")
        .arg(color)
        .arg(font(fontPx))
        .arg(weight);
}

inline QLabel* makeLabel(const QString& text, int fontPx, QWidget* parent, int weight = 400) {
    QLabel* label = new QLabel(text, parent);
    label->setStyleSheet(labelStyle("#000000", fontPx, weight));
    return label;
}

// Keeps the space of a widget in the layout while it is hidden (the "invisible" class).
inline void retainSize(QWidget* widget) {
    QSizePolicy policy = widget->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    widget->setSizePolicy(policy);
}

}  // namespace gcui

enum class GcPhase { FirstHalf, SecondHalf, FirstExtraHalf, SecondExtraHalf, PenaltyShootout };
enum class GcState { Initial, Ready, Set, Playing, Finished, Timeout };

// The game as the frontend sees it, derived from the true control message.
struct GcGame {
        RoboCupGameControlData data{};
        GcPhase phase = GcPhase::FirstHalf;
        GcState state = GcState::Initial;
        int kickingSide = -1;  // 0 = home, 1 = away, -1 = none
        bool stopped = false;

        bool isShootout() const {
            return phase == GcPhase::PenaltyShootout;
        }

        std::optional<RoboCup::Side> kickingTeamSide() const {
            if (kickingSide < 0) {
                return std::nullopt;
            }
            return kickingSide == 0 ? RoboCup::Side::home : RoboCup::Side::away;
        }
};

// Selection shared by both team panels and the penalty panel.
struct GcSelection {
        int penaltyCall = -1;      // index in gcaction::penaltyCalls(), -1 if none is selected
        bool alternative = false;  // shift key: early unpenalize / goalkeeper selection
};

// ActionButton / PenaltyButton of the frontend.
class GcButton : public QPushButton {
    public:
        explicit GcButton(const QString& text, QWidget* parent = nullptr, int fontPx = 16) : QPushButton(parent), fontPx_(fontPx) {
            label_ = new QLabel(text, this);
            label_->setAlignment(Qt::AlignCenter);
            label_->setWordWrap(true);
            label_->setAttribute(Qt::WA_TransparentForMouseEvents);
            QVBoxLayout* layout = new QVBoxLayout(this);
            layout->setContentsMargins(gcui::dp(8), gcui::dp(8), gcui::dp(8), gcui::dp(8));
            layout->addWidget(label_);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setFocusPolicy(Qt::NoFocus);
            restyle();
        }

        void setLabel(const QString& text) {
            if (label_->text() != text) {
                label_->setText(text);
            }
        }

        // Disabled buttons are grayed out, active ones are highlighted.
        void setState(bool legal, bool active = false) {
            if (legal == legal_ && active == active_) {
                return;
            }
            legal_ = legal;
            active_ = active;
            setEnabled(legal);
            restyle();
        }

        QSize sizeHint() const override {
            return QSize(gcui::dp(80), gcui::dp(40));
        }

        QSize minimumSizeHint() const override {
            return QSize(gcui::dp(40), gcui::dp(36));
        }

    private:
        void restyle() {
            QString background = active_ ? "#d1d5db" : (legal_ ? "#ffffff" : "#f3f4f6");
            QString foreground = (active_ || legal_) ? "#000000" : "#6b7280";
            setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #4b5563; border-radius: 6px; }").arg(background));
            label_->setStyleSheet(gcui::labelStyle(foreground, fontPx_));
        }

        QLabel* label_;
        int fontPx_;
        bool legal_ = false;
        bool active_ = false;
};

// PlayerButton of the frontend: jersey color, penalty (or its timer), connection status and number.
class GcPlayerButton : public QPushButton {
    public:
        explicit GcPlayerButton(QWidget* parent = nullptr) : QPushButton(parent) {
            layout_ = new QHBoxLayout(this);
            layout_->setContentsMargins(gcui::dp(16), gcui::dp(4), gcui::dp(16), gcui::dp(4));
            layout_->setSpacing(gcui::dp(16));

            QVBoxLayout* text = new QVBoxLayout();
            text->setSpacing(0);
            colorLabel_ = createLabel(16);
            penaltyLabel_ = createLabel(16);
            text->addWidget(colorLabel_);
            text->addWidget(penaltyLabel_);
            layout_->addLayout(text, 1);

            statusLabel_ = createLabel(24);
            numberLabel_ = createLabel(30);
            layout_->addWidget(statusLabel_);
            layout_->addWidget(numberLabel_);

            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setMinimumHeight(gcui::dp(56));
            setFocusPolicy(Qt::NoFocus);
        }

        // number == 0 shows only the jersey color (selection of the jersey in the shoot-out).
        // connection: 2 = good, 1 = bad, 0 = offline (ConnectionStatusIndicatorIcon).
        void setContent(const QString& color, uint8_t number, const QString& penalty, bool legal, bool flash, int sign,
                        int connection = 0) {
            if (connection != connection_) {
                connection_ = connection;
                statusLabel_->setText(connection >= 2 ? "✓" : connection >= 1 ? "⚠" : "✕");
                statusLabel_->setStyleSheet(gcui::labelStyle(connection >= 2 ? "#16a34a" : connection >= 1 ? "#facc15" : "#dc2626", 24, 700));
            }
            layout_->setDirection(sign > 0 ? QBoxLayout::LeftToRight : QBoxLayout::RightToLeft);
            colorLabel_->setText(gcui::capitalized(color));
            penaltyLabel_->setVisible(number != 0);
            penaltyLabel_->setText(penalty.isEmpty() ? QString(QChar(0x00a0)) : penalty);
            statusLabel_->setVisible(number != 0);
            numberLabel_->setVisible(number != 0);
            numberLabel_->setText(QString::number(number));
            setEnabled(legal);

            QString background = flash ? "#facc15" : gcui::backgroundColor(color);
            QString foreground = legal ? "#000000" : "#6b7280";
            QString style = background + foreground;
            if (style != style_) {
                style_ = style;
                setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #4b5563; border-radius: 6px; }").arg(background));
                colorLabel_->setStyleSheet(gcui::labelStyle(foreground, 16));
                penaltyLabel_->setStyleSheet(gcui::labelStyle(foreground, 16));
                numberLabel_->setStyleSheet(gcui::labelStyle(foreground, 30));
            }
        }

    private:
        QLabel* createLabel(int fontPx) {
            QLabel* label = gcui::makeLabel("", fontPx, this);
            label->setAlignment(Qt::AlignCenter);
            label->setAttribute(Qt::WA_TransparentForMouseEvents);
            return label;
        }

        QHBoxLayout* layout_;
        QLabel* colorLabel_;
        QLabel* penaltyLabel_;
        QLabel* statusLabel_;
        QLabel* numberLabel_;
        QString style_;
        int connection_ = -1;
};

// TeamPanel.jsx
class GcTeamPanel : public QWidget {
    public:
        GcTeamPanel(RoboCup::Side side, GcSelection& selection, std::function<void()> changed, QWidget* parent = nullptr)
            : QWidget(parent), side_(side), index_(side == RoboCup::Side::home ? 0 : 1), selection_(selection), changed_(std::move(changed)) {
            QVBoxLayout* column = new QVBoxLayout(this);
            column->setContentsMargins(0, 0, 0, 0);
            column->setSpacing(gcui::dp(8));

            QHBoxLayout* header = new QHBoxLayout();
            header->setSpacing(gcui::dp(8));
            header->addStretch(1);
            kickingDot_ = gcui::makeLabel("●", 18, this);
            gcui::retainSize(kickingDot_);
            nameLabel_ = gcui::makeLabel("", 24, this, 600);
            header->addWidget(kickingDot_);
            header->addWidget(nameLabel_);
            header->addStretch(1);
            column->addLayout(header);

            substitute_ = createButton("Substitute", [this]() { onSubstituteClicked(); });
            timeout_ = createButton("Timeout", [this]() { gcaction::timeout(side_).apply(); });
            directFreeKick_ = createSetPlayButton("Direct Free Kick", RoboCup::SetPlay::directFreeKick);
            throwIn_ = createSetPlayButton("Throw-in", RoboCup::SetPlay::throwIn);
            goal_ = createButton("Goal", [this]() { gcaction::goal(side_).apply(); });
            indirectFreeKick_ = createSetPlayButton("Indirect Free Kick", RoboCup::SetPlay::indirectFreeKick);
            goalKick_ = createSetPlayButton("Goal Kick", RoboCup::SetPlay::goalKick);
            penaltyKick_ = createSetPlayButton("Penalty Kick", RoboCup::SetPlay::penaltyKick);
            cornerKick_ = createSetPlayButton("Corner Kick", RoboCup::SetPlay::cornerKick);
            createStats();

            grid_ = new QGridLayout();
            grid_->setSpacing(gcui::dp(8));
            column->addLayout(grid_);

            QScrollArea* scroll = new QScrollArea(this);
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setWidgetResizable(true);
            scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");
            QWidget* players = new QWidget();
            players->setAttribute(Qt::WA_StyledBackground, false);
            players->setStyleSheet("background: transparent;");
            QVBoxLayout* playerList = new QVBoxLayout(players);
            playerList->setContentsMargins(0, 0, 0, 0);
            playerList->setSpacing(gcui::dp(8));

            for (bool goalkeeper : {true, false}) {
                GcPlayerButton* button = new GcPlayerButton(players);
                connect(button, &QPushButton::clicked, this, [this, goalkeeper]() {
                    select_ = {Select::PsoPlayer, 0, goalkeeper};
                    changed_();
                });
                playerList->addWidget(button, 1);
                (goalkeeper ? psoGoalkeeperColor_ : psoFieldPlayerColor_) = button;
            }
            for (uint8_t number = 1; number <= RoboCup::PlayerNumber_MAX; ++number) {
                GcPlayerButton* button = new GcPlayerButton(players);
                connect(button, &QPushButton::clicked, this, [this, number]() { onPlayerClicked(number); });
                playerList->addWidget(button, 1);
                players_[number - 1] = button;
            }
            scroll->setWidget(players);
            column->addWidget(scroll, 1);

            setMinimumWidth(gcui::dp(290));
        }

        void refresh(const GcGame& game, int sign) {
            game_ = game;
            if (sign != sign_) {
                sign_ = sign;
                layoutGrid();
            }

            const TeamInfo& team = game.data.teams[index_];
            QString fieldPlayerColor = gcui::colorName(team.fieldPlayerColour);
            QString goalkeeperColor = gcui::colorName(team.goalkeeperColour);
            bool shootout = game.isShootout();

            kickingDot_->setVisible(game.kickingSide == index_);
            nameLabel_->setText(QString::fromStdString(GameControllerBridge::instance().teamName(side_)));
            nameLabel_->setStyleSheet(gcui::labelStyle(gcui::textColor(fieldPlayerColor), 24, 600));

            scoreLabel_->setText(QString::number(team.score));
            scoreLabel_->setAlignment(sign > 0 ? Qt::AlignRight : Qt::AlignLeft);
            if (shootout) {
                messagesTitle_->setText(game.kickingSide == index_ ? "Shot:" : "Shots:");
                messagesValue_->setText(QString::number(team.penaltyShot));
            } else {
                messagesTitle_->setText("Messages:");
                messagesValue_->setText(QString::number(team.messageBudget));
            }
            penaltiesValue_->setText(QString::number(GameControllerBridge::instance().penaltyCounters()[index_]));

            bool select = shootout || selection_.alternative || select_.type == Select::Goalkeeper;
            substitute_->setLabel(select ? "Select" : "Substitute");
            substitute_->setState(true, select_.type != Select::Default);
            timeout_->setState(gcaction::timeout(side_).isLegal());
            goal_->setState(gcaction::goal(side_).isLegal());
            refreshSetPlay(directFreeKick_, RoboCup::SetPlay::directFreeKick, SET_PLAY_DIRECT_FREE_KICK);
            refreshSetPlay(indirectFreeKick_, RoboCup::SetPlay::indirectFreeKick, SET_PLAY_INDIRECT_FREE_KICK);
            refreshSetPlay(penaltyKick_, RoboCup::SetPlay::penaltyKick, SET_PLAY_PENALTY_KICK);
            refreshSetPlay(throwIn_, RoboCup::SetPlay::throwIn, SET_PLAY_THROW_IN);
            refreshSetPlay(goalKick_, RoboCup::SetPlay::goalKick, SET_PLAY_GOAL_KICK);
            refreshSetPlay(cornerKick_, RoboCup::SetPlay::cornerKick, SET_PLAY_CORNER_KICK);

            bool chooseColor = select_.type == Select::PsoColor;
            psoGoalkeeperColor_->setVisible(chooseColor);
            psoFieldPlayerColor_->setVisible(chooseColor);
            if (chooseColor) {
                psoGoalkeeperColor_->setContent(goalkeeperColor, 0, "", true, false, sign);
                psoFieldPlayerColor_->setContent(fieldPlayerColor, 0, "", true, false, sign);
            }

            std::array<std::array<int, MAX_NUM_PLAYERS>, 2> connection = GameControllerBridge::instance().connectionStatus();
            for (uint8_t number = 1; number <= RoboCup::PlayerNumber_MAX; ++number) {
                GcPlayerButton* button = players_[number - 1];
                const RobotInfo& player = team.players[number - 1];
                bool substitute = player.penalty == PENALTY_SUBSTITUTE;
                bool shown = !chooseColor
                             && (select_.type == Select::PsoPlayer ? true : select_.type == Select::PlayerIn ? substitute : !substitute);
                button->setVisible(shown);
                if (!shown) {
                    continue;
                }

                QString color;
                if (select_.type == Select::PsoPlayer) {
                    color = select_.goalkeeper ? goalkeeperColor : fieldPlayerColor;
                } else {
                    uint8_t wearer = select_.type == Select::PlayerIn ? select_.playerOut : number;
                    color = wearer == team.goalkeeper ? goalkeeperColor : fieldPlayerColor;
                }
                bool legal = select_.type != Select::Default || defaultPlayerAction(number).isLegal();

                bool timerStarted = player.penalty != PENALTY_NONE && player.secsTillUnpenalised > 0;
                QString penalty = timerStarted ? gcui::formatMMSS(player.secsTillUnpenalised)
                                               : (player.penalty == PENALTY_NONE ? QString() : gcui::penaltyDescription(player.penalty));
                bool flash = timerStarted && player.secsTillUnpenalised < 10 && player.penalty != PENALTY_SENT_OFF
                             && player.penalty != PENALTY_SUBSTITUTE && player.penalty != PENALTY_MOTION_IN_SET && gcui::flashOn();
                button->setContent(color, number, penalty, legal, flash, sign, connection[index_][number - 1]);
            }
        }

    private:
        enum class Select { Default, Goalkeeper, PlayerOut, PlayerIn, PsoColor, PsoPlayer };

        struct SelectState {
                Select type = Select::Default;
                uint8_t playerOut = 0;
                bool goalkeeper = false;
        };

        GcButton* createButton(const QString& text, std::function<void()> onClick) {
            GcButton* button = new GcButton(text, this);
            button->setMinimumHeight(gcui::dp(56));
            connect(button, &QPushButton::clicked, this, [this, onClick]() {
                onClick();
                changed_();
            });
            return button;
        }

        GcButton* createSetPlayButton(const QString& text, RoboCup::SetPlay setPlay) {
            return createButton(text, [this, setPlay]() { gcaction::startSetPlay(side_, setPlay).apply(); });
        }

        void refreshSetPlay(GcButton* button, RoboCup::SetPlay setPlay, uint8_t code) {
            bool active = game_.data.setPlay == code && game_.kickingSide == index_;
            button->setState(gcaction::startSetPlay(side_, setPlay).isLegal(), active);
        }

        // TeamStats: score, messages (or penalty shots) and penalty counter.
        void createStats() {
            stats_ = new QWidget(this);
            QVBoxLayout* layout = new QVBoxLayout(stats_);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);
            scoreLabel_ = gcui::makeLabel("0", 36, stats_, 700);
            layout->addWidget(scoreLabel_);
            // Label and value on the same line (in the frontend they are on two lines), so that the
            // statistics take less height and the buttons next to them can be smaller.
            messagesTitle_ = gcui::makeLabel("Messages:", 16, stats_);
            messagesValue_ = gcui::makeLabel("", 16, stats_);
            QLabel* penaltiesTitle = gcui::makeLabel("Penalties:", 16, stats_);
            penaltiesValue_ = gcui::makeLabel("0", 16, stats_);
            for (auto [title, value] : {std::pair{messagesTitle_, messagesValue_}, std::pair{penaltiesTitle, penaltiesValue_}}) {
                value->setAlignment(Qt::AlignRight);
                QHBoxLayout* line = new QHBoxLayout();
                line->setSpacing(gcui::dp(8));
                line->addWidget(title);
                line->addStretch(1);
                line->addWidget(value);
                layout->addLayout(line);
            }
            layout->addStretch(1);
        }

        // The buttons of the team are in a 3 x 4 grid whose outer column is next to the border of
        // the window, so the grid is mirrored for the team on the right.
        void layoutGrid() {
            for (QWidget* widget : std::initializer_list<QWidget*>{substitute_, timeout_, directFreeKick_, throwIn_, goal_, indirectFreeKick_,
                                                                   goalKick_, stats_, penaltyKick_, cornerKick_}) {
                grid_->removeWidget(widget);
            }
            int outer = sign_ > 0 ? 0 : 2;
            int inner = sign_ > 0 ? 2 : 0;
            grid_->addWidget(substitute_, 0, outer);
            grid_->addWidget(timeout_, 1, outer);
            grid_->addWidget(directFreeKick_, 2, outer);
            grid_->addWidget(throwIn_, 3, outer);
            grid_->addWidget(goal_, 0, 1, 2, 1);
            grid_->addWidget(indirectFreeKick_, 2, 1);
            grid_->addWidget(goalKick_, 3, 1);
            grid_->addWidget(stats_, 0, inner, 2, 1);
            grid_->addWidget(penaltyKick_, 2, inner);
            grid_->addWidget(cornerKick_, 3, inner);
            for (int i = 0; i < 3; ++i) {
                grid_->setColumnStretch(i, 1);
            }
            for (int i = 0; i < 4; ++i) {
                grid_->setRowStretch(i, 1);
            }
        }

        bool shift() const {
            return QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
        }

        GameControllerAction defaultPlayerAction(uint8_t number) const {
            if (selection_.penaltyCall >= 0) {
                return gcaction::penalize(side_, number, selection_.penaltyCall);
            }
            return gcaction::unpenalize(side_, number, selection_.alternative);
        }

        void onSubstituteClicked() {
            selection_.alternative = shift();
            const TeamInfo& team = game_.data.teams[index_];
            if (select_.type != Select::Default) {
                select_ = {};
            } else if (game_.isShootout()) {
                // If the goalkeeper doesn't have a special jersey color, its selection can be skipped.
                if (team.goalkeeperColour == team.fieldPlayerColour) {
                    select_ = {Select::PsoPlayer, 0, game_.kickingSide != index_};
                } else {
                    select_ = {Select::PsoColor};
                }
            } else if (selection_.alternative) {
                select_ = {Select::Goalkeeper};
            } else {
                select_ = {Select::PlayerOut};
            }
        }

        void onPlayerClicked(uint8_t number) {
            selection_.alternative = shift();
            switch (select_.type) {
                case Select::Default:
                    if (selection_.penaltyCall >= 0) {
                        gcaction::penalize(side_, number, selection_.penaltyCall).apply();
                        if (gcaction::penaltyCalls()[selection_.penaltyCall].second != RoboCup::PenaltyCall::motionInSet) {
                            selection_.penaltyCall = -1;
                        }
                    } else {
                        gcaction::unpenalize(side_, number, selection_.alternative).apply();
                    }
                    break;
                case Select::Goalkeeper:
                    gcaction::selectGoalkeeper(side_, number).apply();
                    select_ = {};
                    break;
                case Select::PlayerOut:
                    select_ = {Select::PlayerIn, number};
                    break;
                case Select::PlayerIn:
                    gcaction::substitute(side_, select_.playerOut, number).apply();
                    select_ = {};
                    break;
                case Select::PsoPlayer:
                    gcaction::selectPenaltyShotPlayer(side_, number, select_.goalkeeper).apply();
                    select_ = {};
                    break;
                case Select::PsoColor:
                    // The jersey buttons have their own handler.
                    break;
            }
            changed_();
        }

        RoboCup::Side side_;
        int index_;
        int sign_ = 0;
        GcSelection& selection_;
        std::function<void()> changed_;
        SelectState select_;
        GcGame game_;

        QLabel* kickingDot_;
        QLabel* nameLabel_;
        QGridLayout* grid_;
        GcButton* substitute_;
        GcButton* timeout_;
        GcButton* directFreeKick_;
        GcButton* throwIn_;
        GcButton* goal_;
        GcButton* indirectFreeKick_;
        GcButton* goalKick_;
        GcButton* penaltyKick_;
        GcButton* cornerKick_;
        QWidget* stats_;
        QLabel* scoreLabel_;
        QLabel* messagesTitle_;
        QLabel* messagesValue_;
        QLabel* penaltiesValue_;
        GcPlayerButton* psoGoalkeeperColor_ = nullptr;
        GcPlayerButton* psoFieldPlayerColor_ = nullptr;
        std::array<GcPlayerButton*, RoboCup::PlayerNumber_MAX> players_{};
};

// CenterPanel.jsx: ClockPanel, StatePanel and PenaltyPanel.
class GcCenterPanel : public QWidget {
    public:
        GcCenterPanel(GcSelection& selection, std::function<void()> changed, QWidget* parent = nullptr)
            : QWidget(parent), selection_(selection), changed_(std::move(changed)) {
            QVBoxLayout* column = new QVBoxLayout(this);
            column->setContentsMargins(0, 0, 0, 0);
            column->setSpacing(gcui::dp(16));

            column->addWidget(createClock());

            QWidget* stateWidget = new QWidget(this);
            stateGrid_ = new QGridLayout(stateWidget);
            stateGrid_->setContentsMargins(0, 0, 0, 0);
            stateGrid_->setSpacing(gcui::dp(8));
            for (int i = 0; i < 5; ++i) {
                stateGrid_->setColumnStretch(i, 1);
            }
            stopResume_ = createStateButton("Stop Play", [this]() { gcaction::stopPlay(game_.stopped).apply(); });
            secondHalf_ = createStateButton("Second Half", []() { gcaction::switchHalf().apply(); });
            shootoutLeft_ = createStateButton("Penalty Shots (Left Goal)", []() {
                gcaction::startPenaltyShootout(RoboCup::SideMapping::homeDefendsRightGoal).apply();
            });
            shootoutRight_ = createStateButton("Penalty Shots (Right Goal)", []() {
                gcaction::startPenaltyShootout(RoboCup::SideMapping::homeDefendsLeftGoal).apply();
            });
            extraTime_ = createStateButton("Extra Time", []() { gcaction::startExtraTime().apply(); });
            ready_ = createStateButton("Ready", [this]() { readyAction().apply(); });
            globalGameStuck_ = createStateButton("Global GS", []() { gcaction::globalGameStuck().apply(); });
            set_ = createStateButton("Set", [this]() { setAction().apply(); });
            playing_ = createStateButton("Playing", [this]() { playingAction().apply(); });
            ballFree_ = createStateButton("Ball Free", []() { gcaction::finishSetPlay().apply(); });
            finish_ = createStateButton("Finish", [this]() { finishAction().apply(); });
            refereeTimeout_ = createStateButton("Referee Timeout", []() { gcaction::timeout(std::nullopt).apply(); });
            column->addWidget(stateWidget);

            QWidget* penaltyWidget = new QWidget(this);
            QGridLayout* penaltyGrid = new QGridLayout(penaltyWidget);
            penaltyGrid->setContentsMargins(0, 0, 0, 0);
            penaltyGrid->setSpacing(gcui::dp(8));
            const auto& calls = gcaction::penaltyCalls();
            for (size_t i = 0; i < calls.size(); ++i) {
                GcButton* button = new GcButton(QString::fromStdString(calls[i].first), penaltyWidget);
                int index = static_cast<int>(i);
                connect(button, &QPushButton::clicked, this, [this, index]() {
                    selection_.penaltyCall = selection_.penaltyCall == index ? -1 : index;
                    changed_();
                });
                penaltyGrid->addWidget(button, index / 2, index % 2);
                penaltyButtons_.push_back(button);
            }
            column->addWidget(penaltyWidget, 1);
        }

        void refresh(const GcGame& game) {
            game_ = game;
            refreshClock();
            refreshState();
            refreshPenalties();
        }

    private:
        QWidget* createClock() {
            QWidget* clock = new QWidget(this);
            QVBoxLayout* layout = new QVBoxLayout(clock);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(0);

            phaseLabel_ = gcui::makeLabel("", 16, clock);
            phaseLabel_->setAlignment(Qt::AlignCenter);
            phaseLabel_->setFixedHeight(gcui::dp(24));
            layout->addWidget(phaseLabel_);

            QHBoxLayout* timer = new QHBoxLayout();
            timer->setSpacing(gcui::dp(8));
            primaryTimer_ = gcui::makeLabel("", 60, clock, 500);
            primaryTimer_->setAlignment(Qt::AlignCenter);
            addAdditionalTime_ = new GcButton("+", clock);
            addAdditionalTime_->setFixedSize(gcui::dp(34), gcui::dp(42));
            connect(addAdditionalTime_, &QPushButton::clicked, this, [this]() {
                gcaction::addAdditionalTime().apply();
                changed_();
            });
            gcui::retainSize(addAdditionalTime_);
            // The "+" button sits at the right of the clock without moving it off center.
            timer->addStretch(1);
            timer->addSpacing(gcui::dp(34));
            timer->addWidget(primaryTimer_);
            QVBoxLayout* plus = new QVBoxLayout();
            plus->setContentsMargins(0, gcui::dp(12), 0, 0);
            plus->addWidget(addAdditionalTime_);
            plus->addStretch(1);
            timer->addLayout(plus);
            timer->addStretch(1);
            layout->addLayout(timer);

            stateLabel_ = gcui::makeLabel("", 16, clock);
            stateLabel_->setAlignment(Qt::AlignCenter);
            stateLabel_->setFixedHeight(gcui::dp(24));
            layout->addWidget(stateLabel_);

            secondaryTimer_ = gcui::makeLabel("", 24, clock);
            secondaryTimer_->setAlignment(Qt::AlignCenter);
            gcui::retainSize(secondaryTimer_);
            layout->addWidget(secondaryTimer_);
            return clock;
        }

        GcButton* createStateButton(const QString& text, std::function<void()> onClick) {
            GcButton* button = new GcButton(text, this);
            button->setMinimumHeight(gcui::dp(56));
            button->hide();
            connect(button, &QPushButton::clicked, this, [this, onClick]() {
                onClick();
                changed_();
            });
            return button;
        }

        bool inHalfTimeBreak() const {
            return ((game_.phase == GcPhase::FirstHalf || game_.phase == GcPhase::FirstExtraHalf) && game_.state == GcState::Finished)
                   || ((game_.phase == GcPhase::SecondHalf || game_.phase == GcPhase::SecondExtraHalf) && game_.state == GcState::Initial);
        }

        // getPhaseDescription in ClockPanel.jsx.
        QString phaseDescription() const {
            switch (game_.phase) {
                case GcPhase::FirstHalf:
                    return game_.state == GcState::Finished ? "Half-Time Break" : "First Half";
                case GcPhase::SecondHalf:
                    return game_.state == GcState::Initial ? "Half-Time Break" : "Second Half";
                case GcPhase::FirstExtraHalf:
                    return game_.state == GcState::Finished ? "Extra Time" : "First Extra Half";
                case GcPhase::SecondExtraHalf:
                    return game_.state == GcState::Initial ? "Extra Time" : "Second Extra Half";
                case GcPhase::PenaltyShootout:
                    return "Penalty Shoot-out";
            }
            return "";
        }

        // getStateDescription in ClockPanel.jsx. The control message reports a kick-off as no set
        // play, so a kick-off is recognized from the state (Ready / Set) or its running timer.
        QString stateDescription() const {
            switch (game_.state) {
                case GcState::Timeout:
                    return "Timeout";
                case GcState::Initial:
                    return "Initial";
                case GcState::Finished:
                    return "Finished";
                default:
                    break;
            }
            QString prefix;
            switch (game_.data.setPlay) {
                case SET_PLAY_DIRECT_FREE_KICK:
                    prefix = "Direct Free Kick";
                    break;
                case SET_PLAY_INDIRECT_FREE_KICK:
                    prefix = "Indirect Free Kick";
                    break;
                case SET_PLAY_PENALTY_KICK:
                    prefix = "Penalty Kick";
                    break;
                case SET_PLAY_THROW_IN:
                    prefix = "Throw-in";
                    break;
                case SET_PLAY_GOAL_KICK:
                    prefix = "Goal Kick";
                    break;
                case SET_PLAY_CORNER_KICK:
                    prefix = "Corner Kick";
                    break;
                default:
                    if (!game_.isShootout()
                        && (game_.state != GcState::Playing || game_.data.secondaryTime != 0)) {
                        prefix = "Kick-off";
                    }
            }
            if (game_.state == GcState::Ready) {
                return prefix + " Ready";
            }
            if (game_.state == GcState::Set) {
                return prefix + " Set";
            }
            return prefix.isEmpty() ? "Playing" : prefix;
        }

        void refreshClock() {
            phaseLabel_->setText(phaseDescription());
            int remaining = game_.data.secsRemaining;
            primaryTimer_->setText(gcui::formatMMSS(remaining));
            bool flash = remaining < 10 && gcui::flashOn();
            primaryTimer_->setStyleSheet(gcui::labelStyle(flash ? "#facc15" : "#000000", 60, 500));
            addAdditionalTime_->setVisible(!game_.isShootout());
            addAdditionalTime_->setState(gcaction::addAdditionalTime().isLegal());
            stateLabel_->setText(stateDescription());
            secondaryTimer_->setVisible(game_.data.secondaryTime != 0);
            secondaryTimer_->setText(gcui::formatMMSS(game_.data.secondaryTime));
        }

        GameControllerAction readyAction() const {
            return gcaction::startSetPlay(game_.kickingTeamSide(), RoboCup::SetPlay::kickOff);
        }

        GameControllerAction setAction() const {
            return game_.isShootout() ? gcaction::waitForPenaltyShot() : gcaction::waitForSetPlay();
        }

        GameControllerAction playingAction() const {
            return game_.isShootout() ? gcaction::freePenaltyShot() : gcaction::freeSetPlay();
        }

        GameControllerAction finishAction() const {
            return game_.isShootout() ? gcaction::finishPenaltyShot() : gcaction::finishHalf();
        }

        // StatePanel.jsx: the buttons that are shown depend on the phase and state of the game, and
        // they are placed in a grid of 5 columns in this order.
        void refreshState() {
            bool shootout = game_.isShootout();
            GcState state = game_.state;
            bool readySetPlaying = state == GcState::Ready || state == GcState::Set || state == GcState::Playing;
            bool firstHalfFinished = (game_.phase == GcPhase::FirstHalf || game_.phase == GcPhase::FirstExtraHalf) && state == GcState::Finished;
            bool hasExtraTime = GameControllerBridge::instance().hasExtraTime();
            bool halfTimeBreak = inHalfTimeBreak();

            std::vector<std::pair<GcButton*, int>> items;
            items.push_back({stopResume_, 5});
            if (halfTimeBreak) {
                items.push_back({secondHalf_, 1});
            }
            if (game_.phase == (hasExtraTime ? GcPhase::SecondExtraHalf : GcPhase::SecondHalf) && state == GcState::Finished) {
                items.push_back({shootoutLeft_, 2});
                items.push_back({shootoutRight_, 2});
            }
            if (game_.phase == GcPhase::SecondHalf && state == GcState::Finished && hasExtraTime) {
                items.push_back({extraTime_, 4});
            }
            if (!shootout && (state == GcState::Initial || state == GcState::Timeout || firstHalfFinished)) {
                items.push_back({ready_, halfTimeBreak ? 3 : 4});
            }
            if (!shootout && state == GcState::Playing) {
                items.push_back({globalGameStuck_, 1});
            }
            if (shootout || state == GcState::Ready || state == GcState::Set) {
                items.push_back({set_, 1});
            }
            if (shootout || readySetPlaying) {
                items.push_back({playing_, shootout ? 2 : 1});
            }
            if (!shootout && readySetPlaying) {
                items.push_back({ballFree_, 1});
            }
            if (shootout || readySetPlaying) {
                items.push_back({finish_, 1});
            }
            items.push_back({refereeTimeout_, 1});
            layoutState(items);

            stopResume_->setLabel(game_.stopped ? "Resume Play" : "Stop Play");
            stopResume_->setState(gcaction::stopPlay(game_.stopped).isLegal(), game_.stopped);
            secondHalf_->setState(gcaction::switchHalf().isLegal());
            shootoutLeft_->setState(gcaction::startPenaltyShootout(RoboCup::SideMapping::homeDefendsRightGoal).isLegal());
            shootoutRight_->setState(gcaction::startPenaltyShootout(RoboCup::SideMapping::homeDefendsLeftGoal).isLegal());
            extraTime_->setState(gcaction::startExtraTime().isLegal());
            ready_->setState(readyAction().isLegal());
            globalGameStuck_->setState(gcaction::globalGameStuck().isLegal());
            set_->setState(setAction().isLegal());
            playing_->setState(playingAction().isLegal());
            ballFree_->setState(gcaction::finishSetPlay().isLegal());
            finish_->setState(finishAction().isLegal());
            refereeTimeout_->setState(gcaction::timeout(std::nullopt).isLegal());
        }

        // Places the buttons like CSS grid auto-placement: row by row, moving to the next row when
        // a button does not fit.
        void layoutState(const std::vector<std::pair<GcButton*, int>>& items) {
            if (items == stateLayout_) {
                return;
            }
            for (const auto& [button, span] : stateLayout_) {
                stateGrid_->removeWidget(button);
                button->hide();
            }
            int row = 0;
            int column = 0;
            for (const auto& [button, span] : items) {
                if (column + span > 5) {
                    ++row;
                    column = 0;
                }
                stateGrid_->addWidget(button, row, column, 1, span);
                button->show();
                column += span;
            }
            stateLayout_ = items;
        }

        // PenaltyPanel.jsx: a call is legal if it can be given to any player.
        void refreshPenalties() {
            for (size_t i = 0; i < penaltyButtons_.size(); ++i) {
                bool legal = false;
                for (RoboCup::Side side : {RoboCup::Side::home, RoboCup::Side::away}) {
                    for (uint8_t number = 1; number <= RoboCup::PlayerNumber_MAX && !legal; ++number) {
                        legal = gcaction::penalize(side, number, i).isLegal();
                    }
                }
                bool selected = selection_.penaltyCall == static_cast<int>(i);
                // A selected penalty call that is not legal anymore is deselected.
                if (selected && !legal) {
                    selection_.penaltyCall = -1;
                    selected = false;
                }
                penaltyButtons_[i]->setState(legal, selected);
            }
        }

        GcSelection& selection_;
        std::function<void()> changed_;
        GcGame game_;

        QLabel* phaseLabel_;
        QLabel* primaryTimer_;
        GcButton* addAdditionalTime_;
        QLabel* stateLabel_;
        QLabel* secondaryTimer_;

        QGridLayout* stateGrid_;
        std::vector<std::pair<GcButton*, int>> stateLayout_;
        GcButton* stopResume_;
        GcButton* secondHalf_;
        GcButton* shootoutLeft_;
        GcButton* shootoutRight_;
        GcButton* extraTime_;
        GcButton* ready_;
        GcButton* globalGameStuck_;
        GcButton* set_;
        GcButton* playing_;
        GcButton* ballFree_;
        GcButton* finish_;
        GcButton* refereeTimeout_;

        std::vector<GcButton*> penaltyButtons_;
};

// Main.jsx: the two team panels around the center panel (mirrored when the home team defends the
// right goal) and the undo panel below.
class GameControllerTool : public Tool {
        Q_OBJECT

    public:
        GameControllerTool(QWidget* parent = nullptr) : Tool(ToolType::GAME_CONTROLLER, parent) {
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

            // The GameController has a light theme, unlike the rest of the simulator: everything is
            // placed in a white container whose rule overrides the dark stylesheets of the Tool and
            // of the window (children with their own stylesheet, such as buttons and labels, keep it).
            QVBoxLayout* outer = new QVBoxLayout(this);
            outer->setContentsMargins(0, 0, 0, 0);
            QWidget* content = new QWidget(this);
            content->setAttribute(Qt::WA_StyledBackground, true);
            content->setStyleSheet("QWidget { background-color: #ffffff; border: none; }");
            outer->addWidget(content);

            QVBoxLayout* root = new QVBoxLayout(content);
            root->setContentsMargins(gcui::dp(8), gcui::dp(8), gcui::dp(8), gcui::dp(8));
            root->setSpacing(gcui::dp(16));

            auto changed = [this]() { refresh(); };
            home_ = new GcTeamPanel(RoboCup::Side::home, selection_, changed, this);
            away_ = new GcTeamPanel(RoboCup::Side::away, selection_, changed, this);
            center_ = new GcCenterPanel(selection_, changed, this);

            top_ = new QHBoxLayout();
            top_->setSpacing(gcui::dp(16));
            placePanels(false);
            root->addLayout(top_, 1);

            // UndoPanel.jsx: the most recent action is on the right.
            QHBoxLayout* undo = new QHBoxLayout();
            undo->setSpacing(gcui::dp(8));
            for (uint32_t states = 1; states <= undoButtons_.size(); ++states) {
                GcButton* button = new GcButton("Undo", this);
                // h-10 in the frontend; a bit taller here so that the scaled label is not clipped.
                button->setFixedHeight(gcui::dp(52));
                connect(button, &QPushButton::clicked, this, [this, states]() {
                    gcaction::undo(states).apply();
                    refresh();
                });
                undoButtons_[states - 1] = button;
            }
            for (auto it = undoButtons_.rbegin(); it != undoButtons_.rend(); ++it) {
                undo->addWidget(*it, 1);
            }
            root->addLayout(undo);

            // Space stops play, as in StatePanel.jsx.
            QShortcut* stopShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
            connect(stopShortcut, &QShortcut::activated, this, [this]() {
                gcaction::stopPlay(false).apply();
                refresh();
            });

            refreshTimer_ = new QTimer(this);
            connect(refreshTimer_, &QTimer::timeout, this, &GameControllerTool::refresh);
            refreshTimer_->start(200);
            refresh();
        }

        // The tool refreshes itself more often than the periodic update of the tools panel.
        void update() override {}

    private:
        void refresh() {
            RoboCupGameControlData data{};
            bool running = GameControllerBridge::instance().readControlData(data, true);
            for (QWidget* widget : std::initializer_list<QWidget*>{home_, center_, away_}) {
                widget->setEnabled(running);
            }
            if (!running) {
                return;
            }

            GcGame game = toGame(data);
            selection_.alternative = QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier;

            // The frontend swaps the team panels with the sides of the field at half-time, but in
            // the simulator the robots keep their side for the whole game (see
            // GameControllerBridge::placeForNewHalf), so only the shoot-out side is followed.
            bool mirror = game.isShootout()
                          && GameControllerBridge::instance().shootoutSides() == RoboCup::SideMapping::homeDefendsRightGoal;
            if (mirror != mirror_) {
                placePanels(mirror);
            }

            home_->refresh(game, mirror ? -1 : 1);
            center_->refresh(game);
            away_->refresh(game, mirror ? 1 : -1);

            std::vector<std::string> names = GameControllerBridge::instance().undoNames(undoButtons_.size());
            for (size_t i = 0; i < undoButtons_.size(); ++i) {
                undoButtons_[i]->setLabel(i < names.size() ? QString::fromStdString(names[i]) : "Undo");
                undoButtons_[i]->setState(gcaction::undo(static_cast<uint32_t>(i + 1)).isLegal());
            }
        }

        GcGame toGame(const RoboCupGameControlData& data) {
            GcGame game;
            game.data = data;
            game.stopped = data.stopped;
            if (data.kickingTeam != KICKING_TEAM_NONE) {
                game.kickingSide = data.teams[0].teamNumber == data.kickingTeam ? 0 : 1;
            }

            switch (data.gamePhase) {
                case GAME_PHASE_NORMAL:
                    lastPhase_ = data.firstHalf ? GcPhase::FirstHalf : GcPhase::SecondHalf;
                    break;
                case GAME_PHASE_EXTRA_TIME:
                    lastPhase_ = data.firstHalf ? GcPhase::FirstExtraHalf : GcPhase::SecondExtraHalf;
                    break;
                case GAME_PHASE_PENALTY_SHOOT_OUT:
                    lastPhase_ = GcPhase::PenaltyShootout;
                    break;
                default:
                    // A timeout keeps the phase in which it was taken.
                    break;
            }
            game.phase = lastPhase_;

            if (data.gamePhase == GAME_PHASE_TIMEOUT) {
                game.state = GcState::Timeout;
            } else {
                switch (data.state) {
                    case STATE_READY:
                        game.state = GcState::Ready;
                        break;
                    case STATE_SET:
                        game.state = GcState::Set;
                        break;
                    case STATE_PLAYING:
                        game.state = GcState::Playing;
                        break;
                    case STATE_FINISHED:
                        game.state = GcState::Finished;
                        break;
                    default:
                        game.state = GcState::Initial;
                }
            }
            return game;
        }

        void placePanels(bool mirror) {
            mirror_ = mirror;
            top_->removeWidget(home_);
            top_->removeWidget(center_);
            top_->removeWidget(away_);
            top_->addWidget(mirror ? away_ : home_, 4);
            top_->addWidget(center_, 5);
            top_->addWidget(mirror ? home_ : away_, 4);
        }

        GcSelection selection_;
        GcPhase lastPhase_ = GcPhase::FirstHalf;
        bool mirror_ = false;

        QHBoxLayout* top_;
        GcTeamPanel* home_;
        GcTeamPanel* away_;
        GcCenterPanel* center_;
        std::array<GcButton*, 5> undoButtons_{};
        QTimer* refreshTimer_;
};

}  // namespace spqr
