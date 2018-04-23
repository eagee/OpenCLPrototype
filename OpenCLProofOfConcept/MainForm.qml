import QtQuick 2.5
import QtQuick.Layouts 1.3
import QtQuick.Controls 1.5

Rectangle {
    id: rectMainForm

    ColumnLayout {
        id: columnLayout1
        spacing: 10
        anchors.rightMargin: 20
        anchors.bottomMargin: 20
        anchors.leftMargin: 20
        anchors.topMargin: 20
        anchors.fill: parent


        Text {
            id: labelDetections
            x: 22
            y: 247
            width: 305
            height: 14
            text: qsTr("Detections: ")
            font.pixelSize: 12
            Layout.fillWidth: true
        }

        TableView {
            id: tableDetections
            sortIndicatorColumn: 1
            sortIndicatorVisible: true
            selectionMode: SelectionMode.SingleSelection
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 5
            Layout.bottomMargin: 20
            model: testScannerModel

            TableViewColumn {
                id: columnFilePath
                role: "filepath"
                title: qsTr("File Path")
                horizontalAlignment: Text.AlignLeft
                width: parent.width
            }
        }

        RowLayout {
            spacing: 10

            Text {
                id: labelCurrentItem
                x: 22
                y: 247
                width: 305
                height: 14
                text: qsTr("Current Item: ") + testScannerModel.currentScanObject
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            Text {
                id: labelElapsedTime
                x: 22
                y: 247
                width: 305
                height: 14
                text: qsTr("Elapsed Time: ") + testScannerModel.timeElapsed
                font.pixelSize: 12
                horizontalAlignment: Qt.AlignRight
                Layout.fillWidth: true
            }
        }



        ProgressBar {
            id: scanProgress
            x: 22
            y: 280
            width: 317
            height: 23
            indeterminate: false
            Layout.fillWidth: true
            Layout.bottomMargin: 20
            value: testScannerModel.scanProgress
        }

        Button {
            id: buttonStart
            x: 252
            y: 309
            text: qsTr("StartScan")
            Layout.preferredWidth: 200
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillWidth: false
            onClicked: {
                testScannerModel.runScan();
            }
        }

    }
}
