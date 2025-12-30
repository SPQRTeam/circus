import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

// Component to display 2D plot data with a single chart and toggle controls
// Can be used for IMU data, position, velocity, or any 3-axis data
Rectangle {
    id: root
    color: "#2a2a2a"

    // Properties for data
    property string title: "2D Plot"
    property int maxDataPoints: 50  // Maximum number of points to display
    property real minValue: -20.0
    property real maxValue: 20.0

    // Settings properties
    property real windowSeconds: 10.0  // Default: last 10 seconds
    property bool autoThresholds: true  // true = automatic, false = manual
    property real manualMinValue: -20.0
    property real manualMaxValue: 20.0

    // Property to expose settings dialog
    property alias settingsDialog: settingsPopup

    // Properties for axis labels
    property string xAxisLabel: "X"
    property string yAxisLabel: "Y"
    property string zAxisLabel: "Z"

    // Visibility toggles for each axis
    property bool showX: true
    property bool showY: true
    property bool showZ: true

    // Arrays to store historical data for each axis
    property var xData: []
    property var yData: []
    property var zData: []
    property var timeData: []
    property real currentTime: 0

    // Function to add new data point
    function addDataPoint(x, y, z) {
        if (appWindow.isSimulationPaused() === true) {
            return; // Do not add data if simulation is running
        }

        // Add new data
        xData.push(x)
        yData.push(y)
        zData.push(z)
        timeData.push(currentTime)
        currentTime += 0.1  // Increment time (adjust based on your update rate)

        // Remove data older than windowSeconds
        var cutoffTime = currentTime - windowSeconds
        while (timeData.length > 0 && timeData[0] < cutoffTime) {
            xData.shift()
            yData.shift()
            zData.shift()
            timeData.shift()
        }

        // Update all series
        updateSeries()
    }

    // Function to update all chart series
    function updateSeries() {
        // Clear all series
        seriesX.clear()
        seriesY.clear()
        seriesZ.clear()

        // Calculate automatic thresholds if enabled
        if (autoThresholds && xData.length > 0) {
            var minVal = Infinity
            var maxVal = -Infinity

            for (var i = 0; i < xData.length; i++) {
                if (showX) {
                    minVal = Math.min(minVal, xData[i])
                    maxVal = Math.max(maxVal, xData[i])
                }
                if (showY) {
                    minVal = Math.min(minVal, yData[i])
                    maxVal = Math.max(maxVal, yData[i])
                }
                if (showZ) {
                    minVal = Math.min(minVal, zData[i])
                    maxVal = Math.max(maxVal, zData[i])
                }
            }

            // Add 10% padding to the range
            var range = maxVal - minVal
            if (range < 0.001) range = 1.0  // Minimum range to avoid division by zero
            var padding = range * 0.1
            axisValue.min = minVal - padding
            axisValue.max = maxVal + padding
        } else {
            // Use manual thresholds
            axisValue.min = manualMinValue
            axisValue.max = manualMaxValue
        }

        // Add all points if visible
        for (var j = 0; j < xData.length; j++) {
            if (showX) seriesX.append(timeData[j], xData[j])
            if (showY) seriesY.append(timeData[j], yData[j])
            if (showZ) seriesZ.append(timeData[j], zData[j])
        }

        // Update axis ranges
        if (timeData.length > 0) {
            var minTime = timeData[0]
            var maxTime = timeData[timeData.length - 1]
            axisTime.min = minTime
            axisTime.max = maxTime
        }
    }

    // Function to clear all data
    function clearData() {
        xData = []
        yData = []
        zData = []
        timeData = []
        currentTime = 0
        updateSeries()
    }

    // Main chart
    ChartView {
        id: chart
        anchors.fill: parent
        anchors.margins: 5
        backgroundColor: "#3a3a3a"
        legend.visible: false
        antialiasing: true
        margins.top: 10
        margins.bottom: 10
        margins.left: 10
        margins.right: 10

        ValueAxis {
            id: axisTime
            titleText: "Time (s)"
            labelFormat: "%.1f"
            labelsColor: "#ffffff"
            gridLineColor: "#555555"
            color: "#ffffff"
            labelsFont.pixelSize: 10
            titleFont.pixelSize: 11
            titleFont.bold: true
            tickCount: Math.floor(root.windowSeconds) + 1
        }

        ValueAxis {
            id: axisValue
            min: root.minValue
            max: root.maxValue
            labelFormat: "%.1f"
            labelsColor: "#ffffff"
            gridLineColor: "#555555"
            color: "#ffffff"
            labelsFont.pixelSize: 10
        }

        LineSeries {
            id: seriesX
            name: root.xAxisLabel
            color: "#ff5555"
            width: 2
            axisX: axisTime
            axisY: axisValue
            visible: root.showX
        }

        LineSeries {
            id: seriesY
            name: root.yAxisLabel
            color: "#55ff55"
            width: 2
            axisX: axisTime
            axisY: axisValue
            visible: root.showY
        }

        LineSeries {
            id: seriesZ
            name: root.zAxisLabel
            color: "#5555ff"
            width: 2
            axisX: axisTime
            axisY: axisValue
            visible: root.showZ
        }
    }

    // Settings popup dialog
    Popup {
        id: settingsPopup
        modal: false
        focus: false
        closePolicy: Popup.CloseOnPressOutside
        x: 0
        y: 0
        width: parent.width
        height: parent.height
        padding: 0

        background: Rectangle {
            color: "#3a3a3a"
            border.color: "#5c8dbd"
            border.width: 2
            radius: 5
        }

        ScrollView {
            anchors.fill: parent
            anchors.margins: 5
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            Item {
                implicitWidth: settingsPopup.width - 10
                implicitHeight: settingsColumn.implicitHeight + 40

                ColumnLayout {
                    id: settingsColumn
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    anchors.topMargin: 20
                    anchors.bottomMargin: 20
                    spacing: 15

                    // Window mode section
                    Label {
                    text: "Window (seconds)"
                    font.pixelSize: 12
                    font.bold: true
                    color: "#cccccc"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        text: "Last"
                        font.pixelSize: 11
                        color: "#aaaaaa"
                    }

                    TextField {
                        id: windowSecondsField
                        Layout.preferredWidth: 100
                        text: root.windowSeconds.toString()
                        font.pixelSize: 11
                        horizontalAlignment: TextInput.AlignHCenter
                        validator: DoubleValidator { bottom: 0.1; decimals: 1 }

                        background: Rectangle {
                            color: "#464545"
                            border.color: parent.activeFocus ? "#5c8dbd" : "#555555"
                            border.width: 1
                            radius: 2
                        }

                        color: "#ffffff"
                    }

                    Label {
                        text: "seconds"
                        font.pixelSize: 11
                        color: "#aaaaaa"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#555555"
                }

                // Thresholds section
                Label {
                    text: "Thresholds"
                    font.pixelSize: 12
                    font.bold: true
                    color: "#cccccc"
                }

                ButtonGroup {
                    id: thresholdModeGroup
                    exclusive: true
                }

                RadioButton {
                    id: radioAutoThresholds
                    text: "Automatic thresholds"
                    checked: root.autoThresholds
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    ButtonGroup.group: thresholdModeGroup

                    indicator: Rectangle {
                        implicitWidth: 18
                        implicitHeight: 18
                        x: radioAutoThresholds.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 9
                        border.color: radioAutoThresholds.checked ? "#5c8dbd" : "#666666"
                        border.width: 2
                        color: "#3a3a3a"

                        Rectangle {
                            width: 10
                            height: 10
                            x: 4
                            y: 4
                            radius: 5
                            color: "#5c8dbd"
                            visible: radioAutoThresholds.checked
                        }
                    }

                    contentItem: Text {
                        text: radioAutoThresholds.text
                        font: radioAutoThresholds.font
                        color: "#aaaaaa"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: radioAutoThresholds.indicator.width + radioAutoThresholds.spacing
                    }
                }

                RadioButton {
                    id: radioManualThresholds
                    text: "Manual thresholds"
                    checked: !root.autoThresholds
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    ButtonGroup.group: thresholdModeGroup

                    indicator: Rectangle {
                        implicitWidth: 18
                        implicitHeight: 18
                        x: radioManualThresholds.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 9
                        border.color: radioManualThresholds.checked ? "#5c8dbd" : "#666666"
                        border.width: 2
                        color: "#3a3a3a"

                        Rectangle {
                            width: 10
                            height: 10
                            x: 4
                            y: 4
                            radius: 5
                            color: "#5c8dbd"
                            visible: radioManualThresholds.checked
                        }
                    }

                    contentItem: Text {
                        text: radioManualThresholds.text
                        font: radioManualThresholds.font
                        color: "#aaaaaa"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: radioManualThresholds.indicator.width + radioManualThresholds.spacing
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    enabled: radioManualThresholds.checked

                    Label {
                        text: "Min:"
                        font.pixelSize: 11
                        color: "#aaaaaa"
                    }

                    TextField {
                        id: minValueField
                        Layout.preferredWidth: 100
                        text: root.manualMinValue.toString()
                        enabled: radioManualThresholds.checked
                        font.pixelSize: 11
                        horizontalAlignment: TextInput.AlignHCenter
                        validator: DoubleValidator { decimals: 2 }

                        background: Rectangle {
                            color: parent.enabled ? "#464545" : "#3a3a3a"
                            border.color: parent.activeFocus ? "#5c8dbd" : "#555555"
                            border.width: 1
                            radius: 2
                        }

                        color: "#ffffff"
                    }

                    Label {
                        text: "Max:"
                        font.pixelSize: 11
                        color: "#aaaaaa"
                    }

                    TextField {
                        id: maxValueField
                        Layout.preferredWidth: 100
                        text: root.manualMaxValue.toString()
                        enabled: radioManualThresholds.checked
                        font.pixelSize: 11
                        horizontalAlignment: TextInput.AlignHCenter
                        validator: DoubleValidator { decimals: 2 }

                        background: Rectangle {
                            color: parent.enabled ? "#464545" : "#3a3a3a"
                            border.color: parent.activeFocus ? "#5c8dbd" : "#555555"
                            border.width: 1
                            radius: 2
                        }

                        color: "#ffffff"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#555555"
                }

                // Visible plots section
                Label {
                    text: "Visible Plots"
                    font.pixelSize: 12
                    font.bold: true
                    color: "#cccccc"
                }

                CheckBox {
                    id: showXCheckbox
                    text: root.xAxisLabel
                    checked: root.showX
                    font.pixelSize: 11

                    indicator: Rectangle {
                        implicitWidth: 18
                        implicitHeight: 18
                        x: showXCheckbox.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 3
                        border.color: "#ff5555"
                        border.width: 2
                        color: showXCheckbox.checked ? "#ff5555" : "transparent"

                        Rectangle {
                            width: 10
                            height: 10
                            x: 4
                            y: 4
                            radius: 2
                            color: "#ffffff"
                            visible: showXCheckbox.checked
                        }
                    }

                    contentItem: Text {
                        text: showXCheckbox.text
                        font: showXCheckbox.font
                        color: "#aaaaaa"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: showXCheckbox.indicator.width + showXCheckbox.spacing
                    }
                }

                CheckBox {
                    id: showYCheckbox
                    text: root.yAxisLabel
                    checked: root.showY
                    font.pixelSize: 11

                    indicator: Rectangle {
                        implicitWidth: 18
                        implicitHeight: 18
                        x: showYCheckbox.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 3
                        border.color: "#55ff55"
                        border.width: 2
                        color: showYCheckbox.checked ? "#55ff55" : "transparent"

                        Rectangle {
                            width: 10
                            height: 10
                            x: 4
                            y: 4
                            radius: 2
                            color: "#ffffff"
                            visible: showYCheckbox.checked
                        }
                    }

                    contentItem: Text {
                        text: showYCheckbox.text
                        font: showYCheckbox.font
                        color: "#aaaaaa"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: showYCheckbox.indicator.width + showYCheckbox.spacing
                    }
                }

                CheckBox {
                    id: showZCheckbox
                    text: root.zAxisLabel
                    checked: root.showZ
                    font.pixelSize: 11

                    indicator: Rectangle {
                        implicitWidth: 18
                        implicitHeight: 18
                        x: showZCheckbox.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 3
                        border.color: "#5555ff"
                        border.width: 2
                        color: showZCheckbox.checked ? "#5555ff" : "transparent"

                        Rectangle {
                            width: 10
                            height: 10
                            x: 4
                            y: 4
                            radius: 2
                            color: "#ffffff"
                            visible: showZCheckbox.checked
                        }
                    }

                    contentItem: Text {
                        text: showZCheckbox.text
                        font: showZCheckbox.font
                        color: "#aaaaaa"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: showZCheckbox.indicator.width + showZCheckbox.spacing
                    }
                }

                Item {
                    Layout.fillHeight: true
                    Layout.preferredHeight: 20
                }

                // Save button
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 35
                    text: "Save"

                    background: Rectangle {
                        color: parent.hovered ? "#5c8dbd" : "#464545"
                        radius: 3
                        border.color: "#5c8dbd"
                        border.width: 1
                    }

                    contentItem: Label {
                        text: parent.text
                        font.pixelSize: 12
                        font.bold: true
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        // Save window settings (in seconds)
                        root.windowSeconds = parseFloat(windowSecondsField.text) || 10.0

                        // Save threshold settings
                        root.autoThresholds = radioAutoThresholds.checked
                        if (!root.autoThresholds) {
                            root.manualMinValue = parseFloat(minValueField.text) || -20.0
                            root.manualMaxValue = parseFloat(maxValueField.text) || 20.0
                        }

                        // Save visibility settings
                        root.showX = showXCheckbox.checked
                        root.showY = showYCheckbox.checked
                        root.showZ = showZCheckbox.checked

                        // Update the plot
                        updateSeries()

                        // Close dialog
                        settingsPopup.close()
                    }
                }
                }
            }
        }
    }
}
