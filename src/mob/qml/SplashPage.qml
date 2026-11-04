// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The logo where the launch screen before it puts its own (gf_splash, in the
// Android package: 96 by 96, its centre 40 above the middle), so one becomes
// the other without a jump; what is said under it never moves it.
Page {
    Image {
        id: logo

        objectName: "splashLogo"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -40
        width: 96
        height: 96
        source: "qrc:/icons/gpgfrontend_logo.png"
        sourceSize: Qt.size(96, 96)
    }

    ColumnLayout {
        anchors.top: logo.bottom
        anchors.topMargin: 16
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 64, 320)
        spacing: 16

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("GpgFrontend")
            font.pixelSize: Math.round(24 * Theme.fontScale)
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
