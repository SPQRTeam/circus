import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

// Component to display 2D plot data with dynamic number of streams
// Can be used for any type of time-series data with configurable streams
Rectangle {
    id: root
    color: "#2a2a2a"

    // Properties for data
    property string title: "2D Plot"
    property int maxDataPoints: 50  // Maximum number of points to display

    // Settings properties
    property real windowSeconds: 10.0  // Default: last 10 seconds
    property bool autoThresholds: true  // true = automatic, false = manual
    property real manualMinValue: -20.0
    property real manualMaxValue: 20.0

    // Property to expose settings dialog
    property alias settingsDialog: settingsPopup

    // Stream configuration
    // Each stream should have: { name: "Stream Name", color: "#ff0000", visible: true }
    property var streams: []

    // Data storage - map of stream name to data array
    property var streamData: ({})
    property var timeData: []
    property real currentTime: 0

    // Function to initialize streams
    function initializeStreams(streamConfigs) {
        streams = streamConfigs
        var newStreamData = {}
        for (var i = 0; i < streamConfigs.length; i++) {
            newStreamData[streamConfigs[i].name] = []
        }
        streamData = newStreamData
        recreateSeries()
    }

    // Function to add new data point
    // dataValues should be an array of values corresponding to the streams
    function addDataPoint(dataValues) {
        if (appWindow.isSimulationPaused() === true) {
            return; // Do not add data if simulation is paused
        }

        if (dataValues.length !== streams.length) {
            console.warn("Data values length mismatch:", dataValues.length, "vs", streams.length)
            return
        }

        // Add new data for each stream
        for (var i = 0; i < streams.length; i++) {
            streamData[streams[i].name].push(dataValues[i])
        }
        timeData.push(currentTime)
        currentTime += 0.1  // Increment time (adjust based on your update rate)

        // Remove data older than windowSeconds
        var cutoffTime = currentTime - windowSeconds
        while (timeData.length > 0 && timeData[0] < cutoffTime) {
            for (var j = 0; j < streams.length; j++) {
                streamData[streams[j].name].shift()
            }
            timeData.shift()
        }

        // Update all series
        updateSeries()
    }

    // Function to recreate all series when stream configuration changes
    function recreateSeries() {
        // Remove all existing series
        chart.removeAllSeries()

        // Create new series for each stream
        for (var i = 0; i < streams.length; i++) {
            var stream = streams[i]
            var series = chart.createSeries(ChartView.SeriesTypeLine, stream.name, axisTime, axisValue)
            series.color = stream.color
            series.width = 2
            series.visible = stream.visible !== undefined ? stream.visible : true
        }

        updateSeries()
    }

    // Function to update all chart series
    function updateSeries() {
        // Calculate automatic thresholds if enabled
        if (autoThresholds && timeData.length > 0) {
            var minVal = Infinity
            var maxVal = -Infinity

            for (var i = 0; i < streams.length; i++) {
                if (streams[i].visible !== false) {
                    var data = streamData[streams[i].name]
                    for (var j = 0; j < data.length; j++) {
                        minVal = Math.min(minVal, data[j])
                        maxVal = Math.max(maxVal, data[j])
                    }
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

        // Update each series
        for (var k = 0; k < chart.count; k++) {
            var series = chart.series(k)
            series.clear()

            // Find corresponding stream
            var streamName = series.name
            var data = streamData[streamName]

            if (data) {
                for (var m = 0; m < data.length; m++) {
                    series.append(timeData[m], data[m])
                }
            }
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
        var newStreamData = {}
        for (var i = 0; i < streams.length; i++) {
            newStreamData[streams[i].name] = []
        }
        streamData = newStreamData
        timeData = []
        currentTime = 0
        updateSeries()
    }

    // Function to update stream visibility
    function setStreamVisibility(streamName, visible) {
        for (var i = 0; i < streams.length; i++) {
            if (streams[i].name === streamName) {
                streams[i].visible = visible
                break
            }
        }

        for (var j = 0; j < chart.count; j++) {
            var series = chart.series(j)
            if (series.name === streamName) {
                series.visible = visible
                break
            }
        }

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
            min: -20.0
            max: 20.0
            labelFormat: "%.1f"
            labelsColor: "#ffffff"
            gridLineColor: "#555555"
            color: "#ffffff"
            labelsFont.pixelSize: 10
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

                    // Dynamic checkboxes for each stream
                    Repeater {
                        model: root.streams

                        CheckBox {
                            id: streamCheckbox
                            text: modelData.name
                            checked: modelData.visible !== undefined ? modelData.visible : true
                            font.pixelSize: 11

                            indicator: Rectangle {
                                implicitWidth: 18
                                implicitHeight: 18
                                x: streamCheckbox.leftPadding
                                y: parent.height / 2 - height / 2
                                radius: 3
                                border.color: modelData.color
                                border.width: 2
                                color: streamCheckbox.checked ? modelData.color : "transparent"

                                Rectangle {
                                    width: 10
                                    height: 10
                                    x: 4
                                    y: 4
                                    radius: 2
                                    color: "#ffffff"
                                    visible: streamCheckbox.checked
                                }
                            }

                            contentItem: Text {
                                text: streamCheckbox.text
                                font: streamCheckbox.font
                                color: "#aaaaaa"
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: streamCheckbox.indicator.width + streamCheckbox.spacing
                            }

                            property string streamName: modelData.name

                            Component.onCompleted: {
                                // Store reference for saving
                                streamCheckbox.objectName = "streamCheckbox_" + index
                            }
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

                            // Save visibility settings for each stream
                            var checkboxRepeater = settingsColumn.children[settingsColumn.children.length - 3]
                            for (var i = 0; i < root.streams.length; i++) {
                                var checkbox = checkboxRepeater.itemAt(i)
                                if (checkbox) {
                                    root.setStreamVisibility(root.streams[i].name, checkbox.checked)
                                }
                            }

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
