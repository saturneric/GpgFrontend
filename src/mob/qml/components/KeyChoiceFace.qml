// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A key to choose, as every key picker shows it: its face (or a key group's
// mark), who it is, and under it how to tell it apart, or a note in its
// place. The picker's own controls (a check box, a button), declared inside
// it, go at the end.
RowLayout {
    id: face

    property string name: ""
    property string email: ""
    property string keyId: ""
    property bool isKey: true
    property bool usable: true
    property var members: []
    // Said instead of the line under the name; in red when @ref problem.
    property string note: ""
    property bool problem: false

    spacing: 12

    KeyAvatar {
        visible: face.isKey
        name: Wording.keyTitle(face.name, face.email, face.keyId)
        usable: face.usable
    }

    // A key group: the desktop's mark for one.
    Rectangle {
        visible: !face.isKey
        implicitWidth: 40
        implicitHeight: 40
        radius: 20
        color: Theme.alpha(Material.accentColor, 0.12)

        Image {
            anchors.centerIn: parent
            source: "qrc:/icons/key-group.png"
            sourceSize: Qt.size(24, 24)
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Label {
            Layout.fillWidth: true
            text: Wording.keyTitle(face.name, face.email, face.keyId)
            elide: Text.ElideRight
            font.pixelSize: Theme.bodySize
        }

        Label {
            Layout.fillWidth: true
            text: face.note.length > 0 ? face.note
                : !face.isKey ? Wording.memberCount((face.members || []).length)
                : Wording.keySubtitle(face.name, face.email, face.keyId)
            elide: Text.ElideRight
            font.pixelSize: Theme.secondarySize
            color: face.problem ? Theme.danger : Material.secondaryTextColor
        }
    }
}
