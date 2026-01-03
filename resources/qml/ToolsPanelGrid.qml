import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Circus 1.0

// Content Area - Resizable Grid
Item {
    id: gridRoot

    // Properties to access parent panel
    required property var panel
    required property var streamToolMap

    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true
    visible: panel.isExpanded

    Rectangle {
        anchors.fill: parent
        color: "#5c5b5b"
        radius: 3

        // Grid Container
        Item {
            anchors.fill: parent
            anchors.margins: 5

            Repeater {
                model: panel.numRows

                Item {
                    id: rowContainer
                    property int rowIndex: index
                    x: 0
                    y: {
                        var yPos = 0
                        for (var i = 0; i < rowIndex; i++) {
                            yPos += parent.height * panel.rowHeights[i]
                        }
                        return yPos
                    }
                    width: parent.width
                    height: parent.height * panel.rowHeights[rowIndex]

                    // Columns in this row
                    Repeater {
                        model: panel.numColumns

                        Item {
                            id: cellContainerWrapper
                            property int colIndex: index
                            x: {
                                var xPos = 0
                                for (var i = 0; i < colIndex; i++) {
                                    xPos += parent.width * panel.columnWidths[i]
                                }
                                return xPos
                            }
                            y: 0
                            width: parent.width * panel.columnWidths[colIndex]
                            height: parent.height

                            // Cell Content
                            ToolsPanelCell {
                                anchors.fill: parent
                                rowIndex: rowContainer.rowIndex
                                colIndex: cellContainerWrapper.colIndex
                                panel: gridRoot.panel
                                streamToolMap: gridRoot.streamToolMap
                            }

                            // Right border resize handle (for columns)
                            Rectangle {
                                visible: cellContainerWrapper.colIndex < panel.numColumns - 1
                                x: parent.width - 2
                                y: 0
                                width: 4
                                height: parent.height
                                color: columnResizeArea.containsMouse || columnResizeArea.pressed ? "#5c8dbd" : "#5c5b5b"
                                z: 10

                                MouseArea {
                                    id: columnResizeArea
                                    anchors.fill: parent
                                    cursorShape: Qt.SizeHorCursor
                                    hoverEnabled: true
                                    preventStealing: true

                                    property real startMouseX: 0
                                    property real startWidth: 0
                                    property real startNextWidth: 0

                                    onPressed: (mouse) => {
                                        startMouseX = mouseX + parent.x
                                        startWidth = panel.columnWidths[cellContainerWrapper.colIndex]
                                        startNextWidth = panel.columnWidths[cellContainerWrapper.colIndex + 1]
                                    }

                                    onPositionChanged: (mouse) => {
                                        if (pressed) {
                                            var currentMouseX = mouseX + parent.x
                                            var deltaX = currentMouseX - startMouseX
                                            var containerWidth = cellContainerWrapper.parent.width
                                            var deltaRatio = deltaX / containerWidth

                                            var newWidths = panel.columnWidths.slice()
                                            var newCurrent = Math.max(0.05, Math.min(0.95, startWidth + deltaRatio))
                                            var newNext = Math.max(0.05, startNextWidth - deltaRatio)

                                            // Ensure we don't go negative
                                            if (newCurrent >= 0.05 && newNext >= 0.05) {
                                                newWidths[cellContainerWrapper.colIndex] = newCurrent
                                                newWidths[cellContainerWrapper.colIndex + 1] = newNext
                                                panel.columnWidths = newWidths
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Bottom border resize handle (for rows)
                    Rectangle {
                        visible: rowContainer.rowIndex < panel.numRows - 1
                        x: 0
                        y: parent.height - 2
                        width: parent.width
                        height: 4
                        color: rowResizeArea.containsMouse || rowResizeArea.pressed ? "#5c8dbd" : "#5c5b5b"
                        z: 10

                        MouseArea {
                            id: rowResizeArea
                            anchors.fill: parent
                            cursorShape: Qt.SizeVerCursor
                            hoverEnabled: true
                            preventStealing: true

                            property real startMouseY: 0
                            property real startHeight: 0
                            property real startNextHeight: 0

                            onPressed: (mouse) => {
                                startMouseY = mouseY + parent.y
                                startHeight = panel.rowHeights[rowContainer.rowIndex]
                                startNextHeight = panel.rowHeights[rowContainer.rowIndex + 1]
                            }

                            onPositionChanged: (mouse) => {
                                if (pressed) {
                                    var currentMouseY = mouseY + parent.y
                                    var deltaY = currentMouseY - startMouseY
                                    var containerHeight = rowContainer.parent.height
                                    var deltaRatio = deltaY / containerHeight

                                    var newHeights = panel.rowHeights.slice()
                                    var newCurrent = Math.max(0.05, Math.min(0.95, startHeight + deltaRatio))
                                    var newNext = Math.max(0.05, startNextHeight - deltaRatio)

                                    // Ensure we don't go negative
                                    if (newCurrent >= 0.05 && newNext >= 0.05) {
                                        newHeights[rowContainer.rowIndex] = newCurrent
                                        newHeights[rowContainer.rowIndex + 1] = newNext
                                        panel.rowHeights = newHeights
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
