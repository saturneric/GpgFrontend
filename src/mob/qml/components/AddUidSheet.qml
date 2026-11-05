// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Another name and address for a key, as the desktop's New UID dialog takes
// it: a name, an email, a comment if wanted.
//
// It stays open while the key is changed: it closes when the user ID is
// added, and keeps what was typed when it was not -- a wrong passphrase is
// said here, and Add tries again.
BottomSheet {
    id: sheet

    // The key is being changed: nothing to tap twice, nothing to close.
    property bool busy: false
    // Why the last try did not work; empty when there is nothing to say.
    property string problem: ""

    signal accepted(string name, string email, string comment)

    function begin() {
        name.clear()
        email.clear()
        comment.clear()
        problem = ""
        open()
    }

    // How the change asked for ended (KeyActionsController's detail).
    function settle(succeeded, detail) {
        if (succeeded) {
            problem = ""
            close()
            return
        }
        // Stopped by the person: nothing went wrong.
        problem = detail.reason === "cancelled" ? "" : (detail.error || "")
    }

    interactive: !busy
    closePolicy: busy ? Popup.NoAutoClose : Popup.CloseOnPressOutside

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

    ProblemLabel {
        objectName: "addUidProblem"
        text: sheet.problem
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        Button {
            text: qsTr("Cancel")
            flat: true
            enabled: !sheet.busy
            onClicked: sheet.close()
        },
        Button {
            objectName: "addUidRun"
            text: sheet.busy ? qsTr("Adding…") : qsTr("Add")
            highlighted: true
            enabled: !sheet.busy && name.text.trim().length > 0
                     && (email.text.trim().length === 0
                         || /^[^@\s]+@[^@\s]+$/.test(email.text.trim()))
            onClicked: {
                sheet.problem = ""
                sheet.accepted(name.text, email.text, comment.text)
            }
        }
    ]
}
