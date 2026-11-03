// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

BottomSheet {
    id: sheet

    // The new key's fingerprint, or a failure.
    signal generated(string fingerprint)

    function begin() {
        name.text = ""
        email.text = ""
        profile.currentIndex = 0
        protect.checked = true
        problem.text = ""
        open()
    }

    function finished(action, succeeded, detail) {
        if (action !== "generate") return
        if (succeeded) {
            sheet.close()
            sheet.generated(detail.fingerprint)
        } else {
            problem.text = qsTr("The key could not be generated: %1")
                           .arg(detail.error)
        }
    }

    objectName: "generateSheet"
    title: qsTr("Create a key")
    closePolicy: actions.busy ? Popup.NoAutoClose
                              : Popup.CloseOnEscape | Popup.CloseOnPressOutside
    interactive: !actions.busy

    KeyActionsController {
        id: actions
    }

    Component.onCompleted: actions.SignalFinished.connect(sheet.finished)

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: qsTr("Others see this name and email when they encrypt to you.")
    }

    TextField {
        id: name
        objectName: "generateName"
        Layout.fillWidth: true
        placeholderText: qsTr("Name")
    }

    TextField {
        id: email
        objectName: "generateEmail"
        Layout.fillWidth: true
        placeholderText: qsTr("Email")
        inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase
    }

    ComboBox {
        id: profile
        Layout.fillWidth: true
        model: actions.profiles
        textRole: "name"
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        font.pixelSize: 12
        text: profile.currentIndex >= 0 && actions.profiles.length > 0
              ? actions.profiles[profile.currentIndex].description : ""
    }

    Switch {
        id: protect
        Layout.fillWidth: true
        Layout.leftMargin: -8
        text: qsTr("Protect it with a passphrase")
        checked: true
    }

    Label {
        id: problem
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Theme.danger
        visible: text.length > 0
    }

    BusyIndicator {
        Layout.alignment: Qt.AlignHCenter
        running: actions.busy
        visible: running
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        Button {
            text: qsTr("Cancel")
            flat: true
            enabled: !actions.busy
            onClicked: sheet.close()
        },
        Button {
            objectName: "generateRun"
            text: qsTr("Create")
            highlighted: true
            enabled: !actions.busy && name.text.trim().length > 0
                     && /^[^@\s]+@[^@\s]+$/.test(email.text.trim())
            onClicked: {
                problem.text = ""
                actions.generate(name.text, email.text,
                                 profile.currentIndex, protect.checked)
            }
        }
    ]
}
