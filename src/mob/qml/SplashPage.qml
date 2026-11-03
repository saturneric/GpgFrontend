// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

Page {
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 320)
        spacing: 16

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 96
            Layout.preferredHeight: 96
            source: "qrc:/icons/gpgfrontend_logo.png"
            sourceSize: Qt.size(96, 96)
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("GpgFrontend")
            font.pixelSize: 24
        }

        ProgressBar {
            Layout.fillWidth: true
            from: 0
            to: 100
            value: StartupController.progress
            indeterminate: StartupController.state !== StartupController.State.kINIT_CORE
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            color: Material.secondaryTextColor
            text: StartupController.state === StartupController.State.kINIT_CORE
                  ? qsTr("Starting the OpenPGP engine…")
                  : qsTr("Opening your profile…")
        }
    }
}
