// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Another name and address for a key, as the desktop's New UID dialog takes
// it: a name, an email, a comment if wanted.
BottomSheet {
    id: sheet

    signal accepted(string name, string email, string comment)

    function begin() {
        name.clear()
        email.clear()
        comment.clear()
        open()
    }

    objectName: "addUidSheet"
    title: qsTr("Add a user ID")

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: qsTr("Another name or address the key stands for. Share the "
                   + "key afterwards, so others learn of it.")
    }

    TextField {
        id: name
        objectName: "uidName"
        Layout.fillWidth: true
        placeholderText: qsTr("Name")
    }

    TextField {
        id: email
        objectName: "uidEmail"
        Layout.fillWidth: true
        placeholderText: qsTr("Email")
        inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase
    }

    TextField {
        id: comment
        objectName: "uidComment"
        Layout.fillWidth: true
        placeholderText: qsTr("Comment (optional)")
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        Button {
            text: qsTr("Cancel")
            flat: true
            onClicked: sheet.close()
        },
        Button {
            objectName: "addUidRun"
            text: qsTr("Add")
            highlighted: true
            enabled: name.text.trim().length > 0
                     && (email.text.trim().length === 0
                         || /^[^@\s]+@[^@\s]+$/.test(email.text.trim()))
            onClicked: {
                sheet.close()
                sheet.accepted(name.text, email.text, comment.text)
            }
        }
    ]
}
