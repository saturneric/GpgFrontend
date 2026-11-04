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

    signal accepted(string purpose, string validity, bool withPassphrase)
    // Expert Mode's choices, KeyGenProfilesController.InfoFromChoices()'s.
    signal customAccepted(var choices, bool withPassphrase)

    function begin() {
        purpose = "encrypt"
        validity = "2y"
        passphrase.checked = true
        // As it was last time, here or on the desktop.
        expert.checked = profileStore.expertMode
        open()
    }

    KeyGenProfilesController {
        id: profileStore
    }

    objectName: "addSubkeySheet"
    title: qsTr("Add a subkey")

    Label {
        Layout.fillWidth: true
        visible: !expert.checked
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: qsTr("A subkey does one job for the key. Its algorithm matches "
                   + "the primary key's.")
    }

    // Every choice, as the desktop's subkey dialog shows them in Expert Mode.
    Switch {
        id: expert
        objectName: "addSubkeyExpert"
        Layout.fillWidth: true
        Layout.leftMargin: -8
        text: qsTr("Expert mode")
        onToggled: profileStore.expertMode = checked
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

    // One list, without the sheet's spacing between its choices.
    ColumnLayout {
        Layout.fillWidth: true
        visible: !expert.checked
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
                Layout.fillWidth: true
                checked: sheet.purpose === modelData.id
                onClicked: sheet.purpose = modelData.id
                text: modelData.text
                Accessible.description: modelData.detail
            }
        }
    }

    Label {
        Layout.fillWidth: true
        Layout.topMargin: 8
        visible: !expert.checked
        text: qsTr("Expires after")
        color: Material.secondaryTextColor
        font.pixelSize: Theme.labelSize
    }

    ComboBox {
        objectName: "subkeyValidity"
        Layout.fillWidth: true
        visible: !expert.checked
        model: sheet.validities
        textRole: "name"
        valueRole: "id"
        // Again once the choices are there: before, there is no index.
        currentIndex: count > 0 ? indexOfValue(sheet.validity) : -1
        onActivated: sheet.validity = currentValue
        Accessible.name: qsTr("Expires after")
    }

    Switch {
        id: passphrase
        objectName: "subkeyPassphrase"
        Layout.fillWidth: true
        text: qsTr("Protect it with a passphrase")
        checked: true
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
            objectName: "addSubkeyRun"
            text: qsTr("Add")
            highlighted: true
            enabled: !expert.checked || chooser.complete
            onClicked: {
                sheet.close()
                if (expert.checked) sheet.customAccepted(chooser.choices, passphrase.checked)
                else sheet.accepted(sheet.purpose, sheet.validity, passphrase.checked)
            }
        }
    ]
}
