// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A whole fingerprint, in groups of four that wrap as the width and the text
// size need, with a button that copies it whole, without the spaces.
ColumnLayout {
    id: view

    property string fingerprint
    property string title: qsTr("Fingerprint")

    spacing: 4

    OutputActions {
        id: output
    }

    RowLayout {
        Layout.fillWidth: true

        Label {
            Layout.fillWidth: true
            text: view.title
            color: Material.secondaryTextColor
            font.pixelSize: Theme.labelSize
        }

        BarButton {
            objectName: "copyFingerprint"
            text: qsTr("Copy the fingerprint")
            icon.source: Theme.icon("content_copy")
            onClicked: {
                output.copy(view.fingerprint.replace(/\s+/g, ""), false)
                copied.visible = true
            }
        }
    }

    Flow {
        objectName: "fingerprint"
        Layout.fillWidth: true
        spacing: 8
        Accessible.role: Accessible.StaticText
        Accessible.name: qsTr("Fingerprint %1")
                         .arg(Theme.fingerprintGroups(view.fingerprint).join(" "))

        Repeater {
            model: Theme.fingerprintGroups(view.fingerprint)

            Label {
                required property string modelData
                text: modelData
                font.family: Theme.mono
                font.pixelSize: Theme.bodySize
            }
        }
    }

    Label {
        id: copied
        visible: false
        text: qsTr("Copied.")
        color: Material.secondaryTextColor
        font.pixelSize: Theme.labelSize
    }
}
