import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Circus 1.0

Item {
    id: cellContainer

    // Required properties
    required property int rowIndex
    required property int colIndex
    required property var panel
    required property Component plot2DComponent

    // ToolCellWrapper instance for this cell
    ToolCellWrapper {
        id: cellWrapper
    }

    // Cell Content
    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        color: "#3a3a3a"
        border.color: "#5c8dbd"
        border.width: 1
        radius: 2

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: {
                contextMenu.popup()
            }
        }

        Menu {
            id: contextMenu
            property var panelRef: panel

            MenuItem {
                text: "Split Horizontally"
                onTriggered: {
                    // Add a new row
                    var panelRef = contextMenu.panelRef
                    panelRef.numRows++
                    var newHeights = panelRef.rowHeights.slice()
                    newHeights.push(1.0 / panelRef.numRows)
                    // Normalize heights
                    var total = newHeights.reduce((a, b) => a + b, 0)
                    for (var i = 0; i < newHeights.length; i++) {
                        newHeights[i] /= total
                    }
                    panelRef.rowHeights = newHeights
                }
            }

            MenuItem {
                text: "Split Vertically"
                onTriggered: {
                    // Add a new column
                    var panelRef = contextMenu.panelRef
                    panelRef.numColumns++
                    var newWidths = panelRef.columnWidths.slice()
                    newWidths.push(1.0 / panelRef.numColumns)
                    // Normalize widths
                    var total = newWidths.reduce((a, b) => a + b, 0)
                    for (var i = 0; i < newWidths.length; i++) {
                        newWidths[i] /= total
                    }
                    panelRef.columnWidths = newWidths
                }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 3

            // Header with cell info and stream selector
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                spacing: 5

                Label {
                    font.pixelSize: 9
                    color: "#888888"
                }

                Button {
                    id: settingsButton
                    Layout.preferredWidth: 25
                    Layout.preferredHeight: 25
                    enabled: plotLoader.item !== null

                    background: Rectangle {
                        color: parent.hovered ? "#5c8dbd" : "#464545"
                        border.color: "#5c8dbd"
                        border.width: 1
                        radius: 2
                    }

                    contentItem: Text {
                        text: "⚙"
                        font.pixelSize: 14
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        if (plotLoader.item && plotLoader.item.settingsDialog) {
                            plotLoader.item.settingsDialog.open()
                        }
                    }
                }

                ComboBox {
                    id: dataStreamCombo
                    Layout.fillWidth: true
                    Layout.preferredHeight: 25
                    editable: true

                    model: panel.dataStreamOptions

                    property string selectedStream: {
                        if (currentIndex >= 0 && currentIndex < model.length) {
                            return model[currentIndex]
                        }
                        return editText || ""
                    }

                    // Update wrapper when stream selection changes
                    onSelectedStreamChanged: {
                        if (selectedStream) {
                            cellWrapper.setStream(selectedStream, panel.teams)
                        }
                    }

                    background: Rectangle {
                        color: parent.pressed ? "#2a2a2a" : (parent.hovered ? "#4a4a4a" : "#464545")
                        border.color: parent.activeFocus ? "#5c8dbd" : "#555555"
                        border.width: 1
                        radius: 2
                    }

                    contentItem: Item {
                        Text {
                            anchors.fill: parent
                            leftPadding: 8
                            rightPadding: dataStreamCombo.indicator.width + 8
                            text: dataStreamCombo.displayText || "Select or type..."
                            font: dataStreamCombo.font
                            color: dataStreamCombo.displayText ? "#ffffff" : "#666666"
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            visible: !textInput.activeFocus && !dataStreamCombo.displayText
                        }

                        TextInput {
                            id: textInput
                            anchors.fill: parent
                            leftPadding: 8
                            rightPadding: dataStreamCombo.indicator.width + 8
                            text: dataStreamCombo.editable ? dataStreamCombo.editText : dataStreamCombo.displayText
                            font: dataStreamCombo.font
                            color: "#ffffff"
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft
                            selectByMouse: true
                            clip: true
                        }
                    }

                    indicator: Canvas {
                        id: canvas
                        x: dataStreamCombo.width - width - 5
                        y: dataStreamCombo.topPadding + (dataStreamCombo.availableHeight - height) / 2
                        width: 12
                        height: 8
                        contextType: "2d"

                        Connections {
                            target: dataStreamCombo
                            function onPressedChanged() { canvas.requestPaint() }
                        }

                        onPaint: {
                            context.reset()
                            context.moveTo(0, 0)
                            context.lineTo(width, 0)
                            context.lineTo(width / 2, height)
                            context.closePath()
                            context.fillStyle = dataStreamCombo.enabled ? "#5c8dbd" : "#666666"
                            context.fill()
                        }
                    }

                    popup: Popup {
                        y: dataStreamCombo.height
                        width: dataStreamCombo.width
                        height: Math.min(200, contentItem.contentHeight + 2)
                        padding: 1
                        z: 3
                        modal: false
                        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: dataStreamCombo.popup.visible ? dataStreamCombo.delegateModel : null
                            currentIndex: dataStreamCombo.highlightedIndex

                            ScrollBar.vertical: ScrollBar {
                                active: true
                                policy: ScrollBar.AsNeeded
                            }
                        }

                        background: Rectangle {
                            color: "#3a3a3a"
                            border.color: "#5c8dbd"
                            border.width: 1
                            radius: 2
                        }
                    }

                    delegate: ItemDelegate {
                        width: dataStreamCombo.width
                        contentItem: Text {
                            text: modelData
                            color: "#ffffff"
                            font: dataStreamCombo.font
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }
                        highlighted: dataStreamCombo.highlightedIndex === index
                        background: Rectangle {
                            color: highlighted ? "#5c8dbd" : (parent.hovered ? "#4a4a4a" : "#3a3a3a")
                        }
                    }
                }
            }

            // Data display area - always visible below ComboBox
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                Layout.minimumHeight: 30
                color: "#5c5b5b"
                radius: 3
                border.color: "#5c8dbd"
                border.width: 1

                Label {
                    anchors.fill: parent
                    anchors.margins: 6
                    font.pixelSize: 11
                    font.family: "monospace"
                    font.bold: true
                    color: "#ffffff"
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    elide: Text.ElideRight
                    wrapMode: Text.NoWrap
                    text: {
                        if (cellWrapper.hasData) {
                            var data = cellWrapper.data
                            if (data && data.x !== undefined) {
                                // Determine labels based on stream type
                                var label1 = "X"
                                var label2 = "Y"
                                var label3 = "Z"
                                if (dataStreamCombo.selectedStream.indexOf("Angular Velocity") >= 0 ||
                                    dataStreamCombo.selectedStream.indexOf("Orientation") >= 0) {
                                    label1 = "R"
                                    label2 = "P"
                                    label3 = "Y"
                                }

                                // If height is small, show on one line
                                if (parent.height < 55) {
                                    return label1 + ":" + data.x.toFixed(3) + " " +
                                           label2 + ":" + data.y.toFixed(3) + " " +
                                           label3 + ":" + data.z.toFixed(3)
                                } else {
                                    // Show on three lines when there's space
                                    return label1 + ": " + data.x.toFixed(4) + "\n" +
                                           label2 + ": " + data.y.toFixed(4) + "\n" +
                                           label3 + ": " + data.z.toFixed(4)
                                }
                            }
                        }
                        return "No data"
                    }
                }

                // Update data periodically
                Timer {
                    interval: 50
                    running: true
                    repeat: true
                    onTriggered: {
                        // Force update of text binding
                    }
                }
            }

            // Plot area - shows Plot2D for data streams
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 100
                color: "#1a1a1a"
                border.color: "#ff0000"
                border.width: 2

                Label {
                    anchors.centerIn: parent
                    text: "Plot area - Loading: " + (plotLoader.status === Loader.Loading ? "Yes" : "No") +
                          "\nReady: " + (plotLoader.status === Loader.Ready ? "Yes" : "No") +
                          "\nItem: " + (plotLoader.item ? "Loaded" : "Null") +
                          "\nStream: " + dataStreamCombo.selectedStream +
                          "\nHas IMU/Pose: " + (dataStreamCombo.selectedStream &&
                                                (dataStreamCombo.selectedStream.indexOf("IMU") >= 0 ||
                                                 dataStreamCombo.selectedStream.indexOf("Pose") >= 0))
                    color: "#ffffff"
                    font.pixelSize: 10
                }

                Loader {
                    id: plotLoader
                    anchors.fill: parent
                    active: true

                    sourceComponent: plot2DComponent

                    onStatusChanged: {
                        console.log("Plot loader status:", status, "item:", item)
                    }

                    onLoaded: {
                        console.log("Plot loaded! Item:", item)
                        if (item) {
                            item.title = dataStreamCombo.selectedStream
                            console.log("Setting plot title:", dataStreamCombo.selectedStream)
                            // Adjust min/max and labels based on stream type
                            if (dataStreamCombo.selectedStream.indexOf("IMU Linear Acceleration") >= 0) {
                                item.minValue = -20.0
                                item.maxValue = 20.0
                                item.xAxisLabel = "X"
                                item.yAxisLabel = "Y"
                                item.zAxisLabel = "Z"
                            } else if (dataStreamCombo.selectedStream.indexOf("IMU Angular Velocity") >= 0) {
                                item.minValue = -10.0
                                item.maxValue = 10.0
                                item.xAxisLabel = "roll"
                                item.yAxisLabel = "pitch"
                                item.zAxisLabel = "yaw"
                            } else if (dataStreamCombo.selectedStream.indexOf("Pose Position") >= 0) {
                                item.minValue = -5.0
                                item.maxValue = 5.0
                                item.xAxisLabel = "X"
                                item.yAxisLabel = "Y"
                                item.zAxisLabel = "Z"
                            } else if (dataStreamCombo.selectedStream.indexOf("Pose Orientation") >= 0) {
                                item.minValue = -3.5
                                item.maxValue = 3.5
                                item.xAxisLabel = "roll"
                                item.yAxisLabel = "pitch"
                                item.zAxisLabel = "yaw"
                            }
                        }
                    }

                    Connections {
                        target: dataStreamCombo
                        function onSelectedStreamChanged() {
                            if (plotLoader.item) {
                                plotLoader.item.title = dataStreamCombo.selectedStream
                                // Update labels and ranges based on stream type
                                if (dataStreamCombo.selectedStream.indexOf("IMU Linear Acceleration") >= 0) {
                                    plotLoader.item.minValue = -20.0
                                    plotLoader.item.maxValue = 20.0
                                    plotLoader.item.xAxisLabel = "X"
                                    plotLoader.item.yAxisLabel = "Y"
                                    plotLoader.item.zAxisLabel = "Z"
                                } else if (dataStreamCombo.selectedStream.indexOf("IMU Angular Velocity") >= 0) {
                                    plotLoader.item.minValue = -10.0
                                    plotLoader.item.maxValue = 10.0
                                    plotLoader.item.xAxisLabel = "roll"
                                    plotLoader.item.yAxisLabel = "pitch"
                                    plotLoader.item.zAxisLabel = "yaw"
                                } else if (dataStreamCombo.selectedStream.indexOf("Pose Position") >= 0) {
                                    plotLoader.item.minValue = -5.0
                                    plotLoader.item.maxValue = 5.0
                                    plotLoader.item.xAxisLabel = "X"
                                    plotLoader.item.yAxisLabel = "Y"
                                    plotLoader.item.zAxisLabel = "Z"
                                } else if (dataStreamCombo.selectedStream.indexOf("Pose Orientation") >= 0) {
                                    plotLoader.item.minValue = -3.5
                                    plotLoader.item.maxValue = 3.5
                                    plotLoader.item.xAxisLabel = "roll"
                                    plotLoader.item.yAxisLabel = "pitch"
                                    plotLoader.item.zAxisLabel = "yaw"
                                }
                                // Clear old data when switching streams
                                plotLoader.item.clearData()
                            }
                        }
                    }

                    // Timer to update plot data
                    Timer {
                        interval: 100  // Update every 100ms
                        running: plotLoader.item !== null
                        repeat: true
                        onTriggered: {
                            if (plotLoader.item && cellWrapper.hasData) {
                                var data = cellWrapper.data
                                if (data && data.x !== undefined) {
                                    plotLoader.item.addDataPoint(data.x, data.y, data.z)
                                }
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillHeight: true
                visible: !plotLoader.item
            }
        }
    }
}
