// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What a profile needs from the person at hand: a new profile's name, the
// passphrase that seals an export (typed twice), or the one that opens an
// import.
BottomSheet {
    id: sheet

    property ProfilesController profiles
    // "create", "export" or "import".
    property string mode: "create"
    readonly property bool busy: profiles !== null && profiles.busy
    readonly property int minLength: profiles ? profiles.minPassphraseLength : 8

    // The name or the passphrase was taken: the page goes on.
    signal accepted(string value)
    // Closed without it.
    signal dismissed()

    property bool taken: false

    function begin(how, error) {
        mode = how
        name.text = ""
        secret.text = ""
        confirm.text = ""
        problem.text = error || ""
        taken = false
        open()
        if (mode === "create") name.forceActiveFocus()
        else secret.forceActiveFocus()
    }

    // Says why the sheet cannot go on, here, or goes on.
    function submit() {
        let why = ""
        if (mode === "create") {
            why = profiles.nameProblem(name.text)
        } else if (mode === "export") {
            if (secret.text.length < minLength)
                why = qsTr("Use at least %1 characters.").arg(minLength)
            else if (secret.text !== confirm.text)
                why = qsTr("The two passphrases are not the same.")
        } else if (secret.text.length === 0) {
            why = qsTr("Enter the passphrase.")
        }
        problem.text = why
        if (why.length > 0) return
        const value = mode === "create" ? name.text : secret.text
        secret.text = ""
        confirm.text = ""
        taken = true
        close()
        accepted(value)
    }

    objectName: "profileSheet"
    title: mode === "create" ? qsTr("New profile")
         : mode === "export" ? qsTr("Export this profile")
         : qsTr("Open the profile file")

    // Typed secrets never stay behind in a closed sheet.
    onClosed: {
        secret.text = ""
        confirm.text = ""
        if (!taken) dismissed()
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: sheet.mode === "create"
              ? qsTr("A profile has its own keys and settings. The new one "
                     + "starts empty.")
              : sheet.mode === "export"
                ? qsTr("The file holds your keys and settings, sealed with "
                       + "this passphrase. Without it, the file cannot be "
                       + "opened, and it cannot be recovered.")
                : qsTr("Enter the passphrase the file was sealed with.")
    }

    TextField {
        id: name
        objectName: "profileName"
        Layout.fillWidth: true
        visible: sheet.mode === "create"
        placeholderText: qsTr("Name, for example Work")
        maximumLength: 64
        onTextEdited: problem.text = ""
        onAccepted: sheet.submit()
    }

    SecretField {
        id: secret
        objectName: "profileSecret"
        Layout.fillWidth: true
        visible: sheet.mode !== "create"
        placeholderText: qsTr("Passphrase")
        onTextEdited: problem.text = ""
        onAccepted: sheet.mode === "export" ? confirm.forceActiveFocus()
                                            : sheet.submit()
    }

    SecretField {
        id: confirm
        objectName: "profileConfirm"
        Layout.fillWidth: true
        visible: sheet.mode === "export"
        placeholderText: qsTr("Passphrase again")
        onTextEdited: problem.text = ""
        onAccepted: sheet.submit()
    }

    ProblemLabel {
        id: problem
        objectName: "profileProblem"
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
            objectName: "profileSubmit"
            text: sheet.mode === "create" ? qsTr("Create")
                : sheet.mode === "export" ? qsTr("Choose where")
                : qsTr("Import")
            highlighted: true
            enabled: !sheet.busy
            onClicked: sheet.submit()
        }
    ]
}
