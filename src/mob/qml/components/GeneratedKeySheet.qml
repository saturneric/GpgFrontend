// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The key just made, once it is made: who it is, what kind of key it is,
// its fingerprint to copy, each of its parts with its algorithm and what it
// does, and when it expires. Shown only for a key that was made: not for a
// failure, nor when it was cancelled.
BottomSheet {
    id: sheet

    // KeyActionsController.describe() of the new key.
    property var key: ({})

    signal openRequested(var key)

    function begin(fingerprint) {
        key = actions.describe(fingerprint)
        if (key.fingerprint === undefined) return
        open()
    }

    objectName: "generatedKeySheet"
    title: qsTr("Key created")

    KeyActionsController {
        id: actions
    }

    readonly property string secretState: key.secretState || ""

    KeyChoiceFace {
        Layout.fillWidth: true
        name: sheet.key.name || ""
        email: sheet.key.email || ""
        keyId: sheet.key.keyId || ""
        secretState: sheet.secretState
        capabilities: sheet.key.capabilities || ""
    }

    InfoRow {
        Layout.fillWidth: true
        label: qsTr("Type")
        value: Wording.keyTag(sheet.secretState) + " · "
               + Wording.keyTagDescription(sheet.secretState)
    }

    // As the key pickers show a key in full: the fingerprint to copy, then
    // each part with what it does.
    KeyFacts {
        objectName: "generatedFacts"
        Layout.fillWidth: true
        key: sheet.key
        parts: sheet.key.subkeys || []
    }

    // Its buttons take a tap only once it is up: it comes where the
    // creation sheet's Create was, and a quick second tap on Create (a key
    // without a passphrase is made in a moment) closed it unseen.
    footer: [
        Item {
            Layout.fillWidth: true
        },
        ActionButton {
            objectName: "generatedOpen"
            kind: "primary"
            enabled: sheet.opened
            text: qsTr("Open the key")
            onClicked: {
                const k = sheet.key
                sheet.close()
                sheet.openRequested(k)
            }
        }
    ]
}
