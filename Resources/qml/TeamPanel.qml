import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: teamPanel

    // Properties
    property string side: "left"  // "left" or "right"
    property string teamName: "Team"
    property var teamRobots: []
    property bool isExpanded: teamRobots.length > 0

    readonly property bool isLeftSide: side === "left"

    Layout.preferredWidth: isExpanded ? 250 : 60
    Layout.fillHeight: true
    color: "#262525"

    Behavior on Layout.preferredWidth {
        NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
    }

    // Border (left side for left panel, right side for right panel)
    Rectangle {
        width: 2
        height: parent.height
        color: "#464545"
        anchors.left: isLeftSide ? parent.right : undefined
        anchors.right: isLeftSide ? undefined : parent.left
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 5

        // Toggle Button
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            background: Rectangle {
                color: "#464545"
                radius: 2
            }

            contentItem: Item {
                implicitWidth: parent.width
                implicitHeight: parent.height

                Label {
                    text: {
                        if (isExpanded) {
                            return (isLeftSide ? "Team 1 <<" : "Team 2 >>")
                        } else {
                            return (isLeftSide ? "T1 >>" : "T2 <<")
                        }
                    }
                    font.pixelSize: 15
                    font.bold: true
                    color: "#ffffff"
                    anchors.centerIn: parent
                }
            }

            onClicked: isExpanded = !isExpanded
        }

        // Content Area
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: teamPanel.width - 10
                spacing: 5

                Repeater {
                    model: teamRobots

                    delegate: Rectangle {
                        Layout.preferredWidth: parent.width
                        Layout.preferredHeight: isExpanded ? (robotExpander.expanded ? robotContent.height + robotHeader.height : robotHeader.height) : 40
                        Layout.alignment: Qt.AlignHCenter
                        color: "#5c5b5b"
                        radius: 3

                        Behavior on Layout.preferredHeight {
                            NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
                        }

                        Column {
                            anchors.fill: parent

                            // Robot Header (Always visible)
                            Rectangle {
                                id: robotHeader
                                width: parent.width
                                height: 40
                                color: "#5c5b5b"
                                radius: 3

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 5

                                    // Left side: show image first (for left panel)
                                    Image {
                                        source: robotExpander.expanded ? "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='%23ecf0f1'%3E%3Cpath d='M7 10l5 5 5-5z'/%3E%3C/svg%3E"
                                                                      : "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='%23ecf0f1'%3E%3Cpath d='M10 17l5-5-5-5z'/%3E%3C/svg%3E"
                                        Layout.preferredWidth: 20
                                        Layout.preferredHeight: 20
                                        visible: isExpanded
                                    }

                                    Label {
                                        text: isExpanded ?
                                            (modelData.name + " (R" + modelData.number + ")") :
                                            "R" + modelData.number
                                        font.pixelSize: isExpanded ? 12 : 14
                                        font.bold: true
                                        color: "#ecf0f1"
                                        Layout.fillWidth: true
                                        horizontalAlignment: isExpanded ? Text.AlignLeft : Text.AlignHCenter
                                    }
                                }

                                MouseArea {
                                    id: robotExpander
                                    anchors.fill: parent
                                    property bool expanded: false
                                    onClicked: {
                                        if (isExpanded) {
                                            expanded = !expanded
                                        } else {
                                            isExpanded = true
                                            expanded = true
                                        }
                                    }
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }

                            // Robot Details (Expandable)
                            Column {
                                id: robotContent
                                width: parent.width * 0.9
                                visible: isExpanded && robotExpander.expanded
                                opacity: (isExpanded && robotExpander.expanded) ? 1 : 0
                                padding: 10
                                spacing: 8

                                Behavior on opacity {
                                    NumberAnimation { duration: 200 }
                                }

                                Label {
                                    text: "Type: " + modelData.type
                                    font.pixelSize: 12
                                    color: "#bdc3c7"
                                    width: parent.width - parent.padding * 2
                                }

                                Label {
                                    text: "Position: [" +
                                          modelData.position[0].toFixed(2) + ", " +
                                          modelData.position[1].toFixed(2) + ", " +
                                          modelData.position[2].toFixed(2) + "]"
                                    font.pixelSize: 12
                                    color: "#bdc3c7"
                                    wrapMode: Text.WordWrap
                                    width: parent.width - parent.padding * 2
                                }

                                Label {
                                    text: "Orientation: [" +
                                          modelData.orientation[0].toFixed(2) + ", " +
                                          modelData.orientation[1].toFixed(2) + ", " +
                                          modelData.orientation[2].toFixed(2) + "]"
                                    font.pixelSize: 12
                                    color: "#bdc3c7"
                                    wrapMode: Text.WordWrap
                                    width: parent.width - parent.padding * 2
                                }

                                Rectangle {
                                    width: parent.width - parent.padding * 2
                                    height: 1
                                    color: "#7f8c8d"
                                }

                                Label {
                                    text: "Properties (Coming Soon)"
                                    font.pixelSize: 11
                                    font.italic: true
                                    color: "#95a5a6"
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
