// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A passphrase the core asks for while an operation runs. Cancelling ends
// the operation as cancelled; going to the background does the same.
Alert {
    id: sheet

    readonly property var request: PassphraseBroker.current
    readonly property bool asking: request.info !== undefined
    // A passphrase being set, typed twice. Not askForNew: rPGP also sets that
    // for symmetric decryption, to keep it out of the passphrase cache.
    readonly property bool choosing: request.confirm === true

    function clear() {
        passphrase.text = ""
        repeat.text = ""
    }

    objectName: "passphraseSheet"
    closePolicy: Popup.NoAutoClose
    visible: asking
    title: choosing ? qsTr("Choose a passphrase") : qsTr("Enter the passphrase")

    onAboutToShow: clear()
    // Ready to type into, keyboard up.
    onOpened: passphrase.forceActiveFocus()

    Component.onCompleted: PassphraseBroker.SignalWipeInput.connect(sheet.clear)

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            visible: text.length > 0
            text: sheet.request.key ? sheet.request.key : ""
            font.weight: Font.DemiBold
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: sheet.request.retry ? Theme.danger : Material.secondaryTextColor
            text: sheet.request.retry
                  ? qsTr("That passphrase was wrong. Try again.")
                  : sheet.choosing
                    ? (sheet.request.key
                       ? qsTr("The passphrase protects this secret key.")
                       : qsTr("Keep it safe: it is needed to use what it "
                              + "protects, and anyone who has it can."))
                    : sheet.request.key
                      ? qsTr("The passphrase unlocks this secret key.")
                      : qsTr("The passphrase this message was encrypted "
                             + "with.")
        }

        SecretField {
            id: passphrase
            objectName: "passphraseField"
            Layout.fillWidth: true
            placeholderText: qsTr("Passphrase")
            onAccepted: if (ok.enabled) ok.clicked()
        }

        SecretField {
            id: repeat
            Layout.fillWidth: true
            visible: sheet.choosing
            placeholderText: qsTr("Repeat the passphrase")
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight

            Button {
                objectName: "passphraseCancel"
                text: qsTr("Cancel")
                flat: true
                onClicked: {
                    sheet.clear()
                    PassphraseBroker.cancel()
                }
            }

            Button {
                id: ok
                objectName: "passphraseOk"
                text: qsTr("OK")
                flat: true
                enabled: passphrase.text.length > 0
                         && (!sheet.choosing || passphrase.text === repeat.text)
                onClicked: {
                    const text = passphrase.text
                    sheet.clear()
                    PassphraseBroker.submit(text)
                }
            }
        }
    }
}
