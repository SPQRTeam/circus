import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform as Platform  // Note: Platform instead of QtQuick.Dialogs

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 720
    title: "SPQR Circus"
    color: "#f1f4f3"

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Open Scene...")
                onTriggered: fileDialog.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Quit")
                onTriggered: Qt.quit()
            }
        }
    }

    Platform.FileDialog {
        id: fileDialog
        title: "Open Scene File"
        folder: "file://" + appWindow.projectRoot + "/Resources/Scenes"
        nameFilters: ["YAML Files (*.yaml)", "All Files (*)"]
        onAccepted: {
            var path = file.toString()
            path = path.replace(/^(file:\/{2})/, "")
            path = decodeURIComponent(path)
            appWindow.loadScene(path)
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        
        // Left Team Panel
        Rectangle {
            id: leftPanel
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            color: "#f1f4f3"
            border.color: "#767676"
            border.width: 2
            
            ScrollView {
                anchors.fill: parent
                anchors.margins: 5
                clip: true
                
                ColumnLayout {
                    width: leftPanel.width - 10
                    spacing: 5
                    
                    Label {
                        text: "Team 1"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#822433"
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        padding: 10
                    }
                    
                    Repeater {
                        model: appWindow.teams.length > 0 ? appWindow.teams[0].robots : []
                        
                        delegate: Rectangle {
                            Layout.preferredWidth: parent.width - 10
                            Layout.preferredHeight: robotExpander.expanded ? robotContent.height + robotHeader.height : robotHeader.height
                            Layout.alignment: Qt.AlignHCenter
                            color: "#822433"
                            radius: 5
                            
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
                                    color: "#822433"
                                    radius: 5
                                    
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 5
                                        
                                        Image {
                                            source: robotExpander.expanded ? "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='%23ecf0f1'%3E%3Cpath d='M7 10l5 5 5-5z'/%3E%3C/svg%3E" 
                                                                          : "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='%23ecf0f1'%3E%3Cpath d='M10 17l5-5-5-5z'/%3E%3C/svg%3E"
                                            Layout.preferredWidth: 20
                                            Layout.preferredHeight: 20
                                        }
                                        
                                        Label {
                                            text: modelData.name + " (#" + modelData.number + ")"
                                            font.pixelSize: 14
                                            font.bold: true
                                            color: "#ecf0f1"
                                            Layout.fillWidth: true
                                        }
                                    }
                                    
                                    MouseArea {
                                        id: robotExpander
                                        anchors.fill: parent
                                        property bool expanded: false
                                        onClicked: expanded = !expanded
                                        cursorShape: Qt.PointingHandCursor
                                    }
                                }
                                
                                // Robot Details (Expandable)
                                Column {
                                    id: robotContent
                                    width: parent.width * 0.9
                                    visible: robotExpander.expanded
                                    opacity: robotExpander.expanded ? 1 : 0
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
        
        // Center - Viewport Container
        Item {
            id: viewportContainer
            objectName: "viewportContainer"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        
        // Right Team Panel
        Rectangle {
            id: rightPanel
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            color: "#f1f4f3"
            border.color: "#767676"
            border.width: 2
            
            ScrollView {
                anchors.fill: parent
                anchors.margins: 5
                clip: true
                
                ColumnLayout {
                    width: rightPanel.width - 10
                    spacing: 5
                    
                    Label {
                        text: "Team 2"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#822433"
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        padding: 10
                    }
                    
                    Repeater {
                        model: appWindow.teams.length > 0 ? appWindow.teams[1].robots : []
                        
                        delegate: Rectangle {
                            Layout.preferredWidth: parent.width - 10
                            Layout.preferredHeight: robotExpander.expanded ? robotContent.height + robotHeader.height : robotHeader.height
                            Layout.alignment: Qt.AlignHCenter
                            color: "#822433"
                            radius: 5
                            
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
                                    color: "transparent"
                                    
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 5
                                        
                                        Image {
                                            source: robotExpander.expanded ? "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='%23ecf0f1'%3E%3Cpath d='M7 10l5 5 5-5z'/%3E%3C/svg%3E" 
                                                                          : "data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='%23ecf0f1'%3E%3Cpath d='M10 17l5-5-5-5z'/%3E%3C/svg%3E"
                                            Layout.preferredWidth: 20
                                            Layout.preferredHeight: 20
                                        }
                                        
                                        Label {
                                            text: modelData.name + " (#" + modelData.number + ")"
                                            font.pixelSize: 14
                                            font.bold: true
                                            color: "#ecf0f1"
                                            Layout.fillWidth: true
                                        }
                                    }
                                    
                                    MouseArea {
                                        id: robotExpander
                                        anchors.fill: parent
                                        property bool expanded: false
                                        onClicked: expanded = !expanded
                                        cursorShape: Qt.PointingHandCursor
                                    }
                                }
                                
                                // Robot Details (Expandable)
                                Column {
                                    id: robotContent
                                    width: parent.width
                                    visible: robotExpander.expanded
                                    opacity: robotExpander.expanded ? 1 : 0
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
}