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

        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Quit")
            highlighted: true
            onClicked: Qt.quit()
        }
    }
}
