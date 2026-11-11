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
            objectName: "pinRepeat"
            Layout.fillWidth: true
            visible: sheet.creating
            placeholderText: qsTr("Repeat the PIN")
            onAccepted: if (unlock.enabled) unlock.clicked()
        }

        // Why Set PIN cannot be pressed yet: too short, as the desktop
        // counts it, then, once the repeat is typed, not the same.
        ProblemLabel {
            objectName: "pinMismatch"
            text: !sheet.creating || pin.text.length === 0 ? ""
                  : pin.text.length < InteractionBridge.minPinLength
                  ? qsTr("Use at least %n characters.", "", InteractionBridge.minPinLength)
                  : repeat.text.length > 0 && pin.text !== repeat.text
                  ? qsTr("The two PINs are not the same.") : ""
        }

        // The buttons at the right, side by side while they fit and on to a
        // new line when they do not (large text, German), Unlock always at
        // the right; one too long for a line wraps its words. Never past
        // the dialog's edge. Laid out from the right: the first is Unlock.
        Flow {
            id: actions

            Layout.fillWidth: true
            layoutDirection: Qt.RightToLeft

            ActionButton {
                id: unlock
                objectName: "pinUnlock"

                width: Math.min(implicitWidth, actions.width)
                kind: "text"
                text: sheet.creating ? qsTr("Set PIN") : qsTr("Unlock")
                enabled: pin.text.length > 0
                         && (!sheet.creating
                             || (pin.text.length >= InteractionBridge.minPinLength
                                 && pin.text === repeat.text))
                onClicked: {
                    InteractionBridge.answer(sheet.request.id,
                                             {"action": "pin", "pin": pin.text})
                    sheet.clear()
                }
            }

            ActionButton {
                objectName: "pinQuit"
                width: Math.min(implicitWidth, actions.width)
                kind: "text"
                text: qsTr("Quit")
                onClicked: {
                    InteractionBridge.decline(sheet.request.id)
                    sheet.clear()
                }
            }

            // After a few wrong ones, as on the desktop: a forgotten PIN can
            // only be replaced, with everything it protected lost.
            ActionButton {
                objectName: "forgotPin"
                width: Math.min(implicitWidth, actions.width)
                visible: !sheet.creating && sheet.request.failures >= 3
                kind: "text"
                text: qsTr("Forgot PIN")
                onClicked: {
                    InteractionBridge.answer(sheet.request.id,
                                             {"action": "reset"})
                    sheet.clear()
                }
            }
        }
    }
}
