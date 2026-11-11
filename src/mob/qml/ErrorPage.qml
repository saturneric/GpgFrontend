// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

Page {

    // Not a stop for a screen reader: a page has no name of its own, only
    // its parts do (Qt gives a Page the role of an unnamed tab).
    Accessible.ignored: true
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 360)
        spacing: 16

        Icon {
            symbol: "error"
            color: Theme.danger
            sourceSize: Qt.size(40, 40)
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("GpgFrontend cannot start")
            font.pixelSize: Theme.titleSize
            wrapMode: Text.Wrap
        }

        Label {
            objectName: "errorMessage"
            Layout.fillWidth: true
            text: StartupController.message
            wrapMode: Text.Wrap
            color: Material.secondaryTextColor
        }

        Label {
            id: restartProblem
            objectName: "restartProblem"
            Layout.fillWidth: true
            visible: text.length > 0
            wrapMode: Text.Wrap
            color: Theme.danger
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: Theme.gapSmall

            // A profile chosen in Settings that will not open: the default
            // one still can, and there is no other way back to it.
            Button {
                objectName: "openDefaultProfile"
                visible: StartupController.chosenProfile
                text: qsTr("Open the default profile")
                flat: true
                onClicked: {
                    if (!StartupController.openDefaultProfile())
                        restartProblem.text = qsTr("GpgFrontend could not start again. "
                                                   + "Quit, then open it again: the "
                                                   + "default profile opens.")
                }
            }

            Button {
                text: qsTr("Quit")
                highlighted: true
                onClicked: Qt.quit()
            }
        }
    }
}
