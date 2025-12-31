import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Toolbar with Add Row button
RowLayout {
    id: header

    // Properties to access parent panel
    required property var panel

    Layout.fillWidth: true
    Layout.preferredHeight: 30
    Layout.topMargin: 0
    Layout.bottomMargin: 5
    Layout.leftMargin: 5
    Layout.rightMargin: 5
    visible: panel.isExpanded
    spacing: 5

    Button {
        text: "Play"
        Layout.preferredHeight: 30
        Layout.preferredWidth: 140
        enabled: panel.isPlaying === false
        background: Rectangle {
            color: parent.hovered ? "#5c8dbd" : "#464545"
            radius: 2
        }
        contentItem: Label {
            text: parent.text
            font.pixelSize: 13
            color: parent.enabled ? "#ffffff" : "#888888"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        onClicked: {
            appWindow.playSimulation()
            panel.isPlaying = appWindow.isSimulationPaused() === false
        }
    }

    Button {
        text: "Pause"
        Layout.preferredHeight: 30
        Layout.preferredWidth: 140
        enabled: panel.isPlaying === true
        background: Rectangle {
            color: parent.hovered ? "#5c8dbd" : "#464545"
            radius: 2
        }
        contentItem: Label {
            text: parent.text
            font.pixelSize: 13
            color: parent.enabled ? "#ffffff" : "#888888"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        onClicked: {
            appWindow.pauseSimulation()
            panel.isPlaying = appWindow.isSimulationPaused() === false
        }
    }

    Item {
        Layout.fillWidth: true
    }

    Button {
        text: "+ Add Row"
        Layout.preferredHeight: 30
        Layout.preferredWidth: 140
        background: Rectangle {
            color: parent.hovered ? "#5c8dbd" : "#464545"
            radius: 2
        }
        contentItem: Label {
            text: parent.text
            font.pixelSize: 13
            color: "#ffffff"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        onClicked: {
            panel.numRows++
            var newHeights = panel.rowHeights.slice()
            newHeights.push(1.0 / panel.numRows)
            // Normalize heights
            var total = newHeights.reduce((a, b) => a + b, 0)
            for (var i = 0; i < newHeights.length; i++) {
                newHeights[i] /= total
            }
            panel.rowHeights = newHeights
        }
    }

    Button {
        text: "- Remove Row"
        Layout.preferredHeight: 30
        Layout.preferredWidth: 160
        enabled: panel.numRows > 1
        background: Rectangle {
            color: parent.enabled ? (parent.hovered ? "#bd5c5c" : "#464545") : "#3a3a3a"
            radius: 2
        }
        contentItem: Label {
            text: parent.text
            font.pixelSize: 13
            color: parent.enabled ? "#ffffff" : "#888888"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        onClicked: {
            if (panel.numRows > 1) {
                panel.numRows--
                var newHeights = panel.rowHeights.slice(0, -1)
                // Normalize heights
                var total = newHeights.reduce((a, b) => a + b, 0)
                for (var i = 0; i < newHeights.length; i++) {
                    newHeights[i] /= total
                }
                panel.rowHeights = newHeights
            }
        }
    }

    Button {
        text: "+ Add Column"
        Layout.preferredHeight: 30
        Layout.preferredWidth: 160
        background: Rectangle {
            color: parent.hovered ? "#5c8dbd" : "#464545"
            radius: 2
        }
        contentItem: Label {
            text: parent.text
            font.pixelSize: 13
            color: "#ffffff"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        onClicked: {
            panel.numColumns++
            var newWidths = panel.columnWidths.slice()
            newWidths.push(1.0 / panel.numColumns)
            // Normalize widths
            var total = newWidths.reduce((a, b) => a + b, 0)
            for (var i = 0; i < newWidths.length; i++) {
                newWidths[i] /= total
            }
            panel.columnWidths = newWidths
        }
    }

    Button {
        text: "- Remove Column"
        Layout.preferredHeight: 30
        Layout.preferredWidth: 160
        enabled: panel.numColumns > 1
        background: Rectangle {
            color: parent.enabled ? (parent.hovered ? "#bd5c5c" : "#464545") : "#3a3a3a"
            radius: 2
        }
        contentItem: Label {
            text: parent.text
            font.pixelSize: 13
            color: parent.enabled ? "#ffffff" : "#888888"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        onClicked: {
            if (panel.numColumns > 1) {
                panel.numColumns--
                var newWidths = panel.columnWidths.slice(0, -1)
                // Normalize widths
                var total = newWidths.reduce((a, b) => a + b, 0)
                for (var i = 0; i < newWidths.length; i++) {
                    newWidths[i] /= total
                }
                panel.columnWidths = newWidths
            }
        }
    }
}
