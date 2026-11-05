// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The logo exactly where the launch screen before it puts its own
// (gf_splash, in the Android package: 96 by 96, its centre 27 above the
// middle of the whole screen, status bar included), on the same colour, so
// one becomes the other without a jump. What is said under it never moves
// it, and comes only when the start takes long enough to need saying, fading
// in: a quick start shows the logo alone, as if nothing changed.
Page {
    id: page

    // Not a stop for a screen reader: a page has no name of its own, only
    // its parts do (Qt gives a Page the role of an unnamed tab).
    Accessible.ignored: true

    // The launch screen's logo centre, from the top of the whole window,
    // where the page's own top is below the status bar.
    readonly property real logoCentre: (Window.height > 0 ? Window.height : height) / 2
                                       - 27 - Theme.safeTop

    background: Rectangle {
        // gf_splash_colors.xml: the launch screen's own colours.
        color: Theme.dark ? "#1C1B1F" : "#FFFBFE"
    }

    Image {
        id: logo

        objectName: "splashLogo"
        anchors.horizontalCenter: parent.horizontalCenter
        y: page.logoCentre - height / 2
        width: 96
        height: 96
        source: "qrc:/icons/gpgfrontend_logo.png"
        sourceSize: Qt.size(96, 96)
    }

    ColumnLayout {
        id: progress

        objectName: "splashProgress"
        anchors.top: logo.bottom
        anchors.topMargin: 16
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 64, 320)
        spacing: 16
        opacity: 0

        // Only a start that takes a while says what it is doing.
        SequentialAnimation on opacity {
            running: true
            PauseAnimation { duration: 500 }
            NumberAnimation { to: 1; duration: 250; easing.type: Easing.OutCubic }
        }

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
