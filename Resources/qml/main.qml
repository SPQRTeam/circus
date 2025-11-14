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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        Item {
            id: viewportContainer
            objectName: "viewportContainer"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}