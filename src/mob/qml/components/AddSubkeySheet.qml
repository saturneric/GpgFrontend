// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A new subkey, as the desktop adds one: what it is for -- the algorithm
// follows from that and from the primary key -- how long it lasts, and
// whether a passphrase protects it.
BottomSheet {
    id: sheet

    // [{id, name}], as KeyActionsController.validities gives them.
    property var validities: []
    // The key the subkey is for: what may be made under it.
    property string fingerprint: ""
    property string purpose: "encrypt"
    property string validity: "2y"
    // The key is being changed: nothing to tap twice, nothing to close.
    property bool busy: false
    // Why the last try did not work; empty when there is nothing to say.
    property string problem: ""
    // The last try failed at the passphrase: the same choices can be tried
    // again at once, asked for the passphrase anew.
    property bool retry: false

    signal accepted(string purpose, string validity, bool withPassphrase)
    // Expert Mode's choices, KeyGenProfilesController.InfoFromChoices()'s.
    signal customAccepted(var choices, bool withPassphrase)

    function begin() {
        purpose = "encrypt"
        validity = "2y"
        passphrase.checked = true
        // As it was last time, here or on the desktop.
        expert.checked = profileStore.expertMode
        problem = ""
        retry = false
        open()
    }

    // A choice changed: what went wrong with the last one no longer applies.
    function changed() {
        problem = ""
        retry = false
    }

    // How the addition asked for ended (KeyActionsController's detail): the
    // sheet goes once it is done; otherwise it stays, as it was filled in,
    // saying why, for another try.
    function settle(succeeded, detail) {
        if (succeeded) {
            changed()
            close()
            return
        }
        // Stopped by the person: nothing went wrong. A passphrase is said to
        // be wrong only when the engine said so (ErrorWording).
        problem = detail.reason === "cancelled" ? "" : (detail.error || "")
        retry = detail.reason === "bad_passphrase"
    }

    interactive: !busy
    closePolicy: busy ? Popup.NoAutoClose : Popup.CloseOnPressOutside

    KeyGenProfilesController {
        id: profileStore
    }

    objectName: "addSubkeySheet"
    title: qsTr("Add a subkey")

    // Every choice, as the desktop's subkey dialog shows them in Expert Mode.
    // First: switching it shows or hides one block below, nothing else moves.
    SettingSwitch {
        id: expert
        objectName: "addSubkeyExpert"
        text: qsTr("Expert mode")
        onToggled: {
            profileStore.expertMode = checked
            sheet.changed()
        }
    }

    // What it is for, side by side while they fit; the algorithm follows
    // from it and from the primary key.
    ColumnLayout {
        Layout.fillWidth: true
        visible: !expert.checked
        spacing: 2

        Label {
            text: qsTr("Used for")
            color: Material.secondaryTextColor
            font.pixelSize: Theme.labelSize
        }

        Flow {
            objectName: "subkeyPurposes"
            Layout.fillWidth: true
            spacing: 0

            Repeater {
                model: [
                    {"id": "encrypt", "text": qsTr("Encryption"),
                     "detail": qsTr("Others encrypt to it; you decrypt with it.")},
                    {"id": "sign", "text": qsTr("Signing"),
                     "detail": qsTr("You sign messages and files with it.")},
                    {"id": "authenticate", "text": qsTr("Authentication"),
                     "detail": qsTr("You log in with it, over SSH for one.")}
                ]

                RadioButton {
                    required property var modelData
                    objectName: "subkeyPurpose_" + modelData.id
                    checked: sheet.purpose === modelData.id
                    onClicked: {
                        sheet.purpose = modelData.id
                        sheet.changed()
                    }
                    text: modelData.text
                    leftPadding: 0
                    Accessible.description: modelData.detail
                }
            }
        }

        Label {
            objectName: "subkeyPurposeDetail"
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: Material.secondaryTextColor
            font.pixelSize: Theme.secondarySize
            text: sheet.purpose === "sign" ? qsTr("You sign messages and files with it.")
                : sheet.purpose === "authenticate"
                  ? qsTr("You log in with it, over SSH for one.")
                  : qsTr("Others encrypt to it; you decrypt with it.")
        }
    }

    KeyAlgoChooser {
        id: chooser
        objectName: "addSubkeyChooser"
        Layout.fillWidth: true
        visible: expert.checked
        profiles: profileStore
        // Asked when shown: what fits under this key.
        algos: expert.checked && sheet.fingerprint.length > 0
               ? profileStore.subkeyAlgosFor(sheet.fingerprint) : []
        subkey: true
        validities: sheet.validities
    }

    // How long it lasts: its label beside its choice, or above it when the
    // text is large. The choice is as wide as its longest option.
    GridLayout {
        Layout.fillWidth: true
        visible: !expert.checked
        columns: Theme.fontScale > 1.3 ? 1 : 2
        columnSpacing: Theme.gap
        rowSpacing: 2

        Label {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            text: qsTr("Expires after")
            wrapMode: Text.Wrap
        }

        ChoiceBox {
            objectName: "subkeyValidity"
            Layout.alignment: Qt.AlignVCenter
                              | (Theme.fontScale > 1.3 ? Qt.AlignLeft : Qt.AlignRight)
            Layout.maximumWidth: sheet.width - 2 * Theme.sheetPad
            model: sheet.validities
            textRole: "name"
            valueRole: "id"
            // Again once the choices are there: before, there is no index.
            currentIndex: count > 0 ? indexOfValue(sheet.validity) : -1
            onActivated: {
                sheet.validity = currentValue
                sheet.changed()
            }
            Accessible.name: qsTr("Expires after")
        }
    }

    SettingSwitch {
        id: passphrase
        objectName: "subkeyPassphrase"
        text: qsTr("Protect it with a passphrase")
        checked: true
        onToggled: sheet.changed()
    }

    // Why the last try failed, above the buttons and never scrolled away:
    // the button under it tries again.
    status: ProblemLabel {
        objectName: "addSubkeyProblem"
        text: sheet.problem
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        ActionButton {
            kind: "text"
            text: qsTr("Cancel")
            enabled: !sheet.busy
            onClicked: sheet.close()
        },
        ActionButton {
            objectName: "addSubkeyRun"
            text: sheet.busy ? qsTr("Adding…")
                  : sheet.retry ? qsTr("Try again") : qsTr("Add")
            enabled: !sheet.busy && (!expert.checked || chooser.complete)
            onClicked: {
                sheet.changed()
                if (expert.checked) sheet.customAccepted(chooser.choices, passphrase.checked)
                else sheet.accepted(sheet.purpose, sheet.validity, passphrase.checked)
            }
        }
    ]
}
