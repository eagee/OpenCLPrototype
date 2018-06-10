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
            spacing: 20
            Layout.bottomMargin: 20
            Layout.alignment: Qt.AlignLeft
            visible: !testScannerModel.running
            Text {
                id: labelOptions
                x: 22
                y: 247
                width: 305
                height: 14
                text: qsTr("Options: ")
                font.pixelSize: 12
                Layout.fillWidth: true
            }

            CheckBox {
                id: checkUseGPU
                text: qsTr("Use GPU")
                checked: testScannerModel.useGPU
                Layout.alignment: Qt.AlignLeft
                onCheckedChanged: {
                    testScannerModel.useGPU = checkUseGPU.checked;
                }
            }

            Rectangle {
                Layout.preferredWidth: 500
                Layout.fillWidth: true
            }
        }

        RowLayout {
            spacing: 10
            //visible: testScannerModel.itemsScanned > 0
            Layout.bottomMargin: testScannerModel.running ? 0 : 20
            Text {
                id: labelCurrentItem
                x: 22
                y: 247
                width: 305
                height: 14
                text: qsTr("Current Item: ") + testScannerModel.currentScanObject
                font.pixelSize: 12
                visible: testScannerModel.running
                Layout.fillWidth: true
            }

            Text {
                id: labelItemsScanned
                x: 22
                y: 247
                width: 305
                height: 14
                text: qsTr("Items Scanned: ") + testScannerModel.itemsScanned
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
                Connections
                {
                    target: testScannerModel
                    onTimeElapsedChanged: {
                        labelElapsedTime.text = testScannerModel.timeElapsed;
                    }
                }
            }
        }

        ProgressBar {
            id: scanProgress
            visible: testScannerModel.running
            x: 22
            y: 280
            width: 317
            height: 23
            indeterminate: false
            Layout.fillWidth: true
            Layout.bottomMargin: 20
            value: 0
            Connections {
                target: testScannerModel
                onScanProgressChanged: {
                    scanProgress.value = testScannerModel.scanProgress;
                }
            }
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
