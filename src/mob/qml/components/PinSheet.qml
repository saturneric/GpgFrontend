// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The application key's PIN, asked by the profile loader. Quitting is the
// default; nothing is guessed.
Alert {
    id: sheet

    readonly property var request: InteractionBridge.current
    readonly property bool asking: request.kind === "app_key_pin"
    readonly property bool creating: asking && request.creating === true

    function clear() {
        pin.text = ""
        repeat.text = ""
    }

    closePolicy: Popup.NoAutoClose
    visible: asking
    title: creating ? qsTr("Choose a PIN") : qsTr("Enter your PIN")

    onAboutToShow: clear()
    // Ready to type into, keyboard up.
    onOpened: pin.forceActiveFocus()

    // Going to the background forgets whatever was typed.
    Component.onCompleted: InteractionBridge.SignalWipeSecretInputs.connect(sheet.clear)
    // The bridge outlives the sheet: let go of it with the sheet.
    Component.onDestruction: InteractionBridge.SignalWipeSecretInputs.disconnect(sheet.clear)

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: !sheet.creating && sheet.request.failures > 0
                   ? Theme.danger : Material.secondaryTextColor
            text: sheet.creating
                  ? qsTr("The PIN protects the key that encrypts this profile.")
                  : sheet.request.failures > 0
                    ? qsTr("That PIN was wrong. Try again.")
                    : qsTr("Your profile is protected by a PIN.")
        }

        SecretField {
            id: pin
            objectName: "pinField"
            Layout.fillWidth: true
            placeholderText: qsTr("PIN")
            // A new PIN goes on to its repeat; only a button that is enabled
            // is pressed, so Enter never sets one nobody confirmed.
            onAccepted: {
                if (sheet.creating) repeat.forceActiveFocus()
                else if (unlock.enabled) unlock.clicked()
            }
        }

        SecretField {
            id: repeat
            Layout.fillWidth: true
            visible: sheet.creating
            placeholderText: qsTr("Repeat the PIN")
            onAccepted: if (unlock.enabled) unlock.clicked()
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight

            // After a few wrong ones, as on the desktop: a forgotten PIN can
            // only be replaced, with everything it protected lost.
            Button {
                objectName: "forgotPin"
                visible: !sheet.creating && sheet.request.failures >= 3
                text: qsTr("Forgot PIN")
                flat: true
                onClicked: {
                    InteractionBridge.answer(sheet.request.id,
                                             {"action": "reset"})
                    sheet.clear()
                }
            }

            Button {
                text: qsTr("Quit")
                flat: true
                onClicked: {
                    InteractionBridge.decline(sheet.request.id)
                    sheet.clear()
                }
            }

            Button {
                id: unlock
                text: sheet.creating ? qsTr("Set PIN") : qsTr("Unlock")
                flat: true
                enabled: pin.text.length > 0
                         && (!sheet.creating || pin.text === repeat.text)
                onClicked: {
                    InteractionBridge.answer(sheet.request.id,
                                             {"action": "pin", "pin": pin.text})
                    sheet.clear()
                }
            }
        }
    }
}
