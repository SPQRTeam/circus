import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Circus 1.0

Rectangle {
    id: toolsPanel

    property bool isExpanded: true
    property int minHeight: 500
    property int maxHeight: Screen.height - 100
    property int expandedHeight: 500
    property int collapsedHeight: 40

    // Grid properties
    property int numColumns: 1
    property int numRows: 1
    property var columnWidths: [1.0]
    property var rowHeights: [1.0]

    // Play/Pause state
    property bool isPlaying: true

    // Teams data
    property var teams: []
    property var dataStreamOptions: buildDataStreamOptions()

    // Component for Plot2D
    Component {
        id: plot2DComponent
        Plot2D {
            id: plot2d
        }
    }

    // Build list of available data streams from all robots
    function buildDataStreamOptions() {
        var options = []

        for (var teamIdx = 0; teamIdx < teams.length; teamIdx++) {
            var team = teams[teamIdx]
            if (!team || !team.robots) continue

            var teamName = "Team " + (teamIdx + 1)

            for (var robotIdx = 0; robotIdx < team.robots.length; robotIdx++) {
                var robot = team.robots[robotIdx]
                if (!robot) continue

                var robotPrefix = teamName + " - Robot " + robot.number

                // Add IMU data streams
                if (robot.imu) {
                    if (robot.imu.linearAcceleration) {
                        options.push(robotPrefix + ": Linear Acceleration")
                    }
                    if (robot.imu.angularVelocity) {
                        options.push(robotPrefix + ": Angular Velocity")
                    }
                }

                // Add Pose data streams
                if (robot.pose) {
                    if (robot.pose.position) {
                        options.push(robotPrefix + ": Position")
                    }
                    if (robot.pose.orientation) {
                        options.push(robotPrefix + ": Orientation")
                    }
                }

                // Add Camera Image stream
                if (robot.image) {
                    if (robot.image.left_camera) {
                        options.push(robotPrefix + ": Left Camera Image")
                    }
                    if (robot.image.right_camera) {
                        options.push(robotPrefix + ": Right Camera Image")
                    }
                }
            }
        }

        console.log("Built data stream options:", options)
        return options
    }

    // Rebuild options when teams change
    onTeamsChanged: {
        dataStreamOptions = buildDataStreamOptions()
    }

    Layout.fillWidth: true
    Layout.preferredHeight: expandedHeight
    color: "#262525"

    Behavior on Layout.preferredHeight {
        enabled: !resizeMouseArea.pressed
        NumberAnimation { duration: 10; easing.type: Easing.InOutQuad }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        // Toggle Button with Resize Handle
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            Layout.topMargin: 2
            Layout.bottomMargin: 5
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            background: Rectangle {
                color: resizeMouseArea.containsMouse || resizeMouseArea.pressed ? "#5c8dbd" : "#464545"
                radius: 2

                Behavior on color {
                    ColorAnimation { duration: 150 }
                }
            }

            contentItem: Label {
                text: "Tools Panel"
                font.pixelSize: 14
                font.bold: true
                color: "#ffffff"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                id: resizeMouseArea
                anchors.fill: parent
                cursorShape: Qt.SizeVerCursor
                hoverEnabled: true
                preventStealing: true

                property real startMouseY: 0
                property real startPanelHeight: 0
                property bool isDragging: false

                onPressed: (mouse) => {
                    startMouseY = mouse.y + parent.y
                    startPanelHeight = toolsPanel.Layout.preferredHeight
                    isDragging = false
                    mouse.accepted = true
                }

                onPositionChanged: (mouse) => {
                    if (pressed) {
                        var currentMouseY = mouse.y + parent.y
                        var deltaY = startMouseY - currentMouseY

                        // If moved more than 5 pixels, consider it a drag
                        if (Math.abs(deltaY) > 1) {
                            isDragging = true

                            // Expand if collapsed
                            if (!toolsPanel.isExpanded) {
                                toolsPanel.isExpanded = true
                            }

                            var newHeight = Math.max(toolsPanel.minHeight, Math.min(toolsPanel.maxHeight, startPanelHeight + deltaY))
                            toolsPanel.Layout.preferredHeight = newHeight
                            toolsPanel.expandedHeight = newHeight
                        }
                    }
                }

                onReleased: {
                    if (!isDragging) {
                        // If not dragging, treat as a click to toggle
                        toolsPanel.isExpanded = !toolsPanel.isExpanded
                        if (toolsPanel.isExpanded) {
                            toolsPanel.Layout.preferredHeight = toolsPanel.expandedHeight
                        } else {
                            toolsPanel.Layout.preferredHeight = toolsPanel.collapsedHeight
                        }
                    } else {
                        // Snap to collapsed if close to minHeight
                        if (toolsPanel.Layout.preferredHeight < toolsPanel.minHeight + 30) {
                            toolsPanel.isExpanded = false
                            toolsPanel.Layout.preferredHeight = toolsPanel.collapsedHeight
                        } else {
                            toolsPanel.expandedHeight = toolsPanel.Layout.preferredHeight
                        }
                    }
                }
            }
        }

        // Toolbar Header
        ToolsPanelHeader {
            panel: toolsPanel
        }

        // Content Area - Resizable Grid
        ToolsPanelGrid {
            panel: toolsPanel
            plot2DComponent: plot2DComponent
        }
    }
}
