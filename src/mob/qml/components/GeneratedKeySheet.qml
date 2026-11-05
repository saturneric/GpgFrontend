// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The key just made, once it is made: who it is, its fingerprint to copy,
// what kind of key it is, its algorithm and when it expires. Shown only for
// a key that was made: not for a failure, nor when it was cancelled.
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
    }

    FingerprintView {
        Layout.fillWidth: true
        fingerprint: sheet.key.fingerprint || ""
    }

    InfoRow {
        Layout.fillWidth: true
        label: qsTr("Type")
        value: Wording.keyTag(sheet.secretState) + " · "
               + Wording.keyTagDescription(sheet.secretState)
    }

    InfoRow {
        Layout.fillWidth: true
        label: qsTr("Algorithm")
        value: [sheet.key.algorithmDetail || sheet.key.algorithm || ""]
               .concat((sheet.key.subkeys || []).slice(1).map(s => s.algorithmDetail
                                                              || s.algorithm || ""))
               .filter(t => t.length > 0).join(" + ")
    }

    InfoRow {
        Layout.fillWidth: true
        label: qsTr("Expires")
        value: Theme.date(sheet.key.expires) || qsTr("Never")
    }

    // Its buttons take a tap only once it is up: it comes where the
    // creation sheet's Create was, and a quick second tap on Create (a key
    // without a passphrase is made in a moment) closed it unseen.
    footer: [
        ActionButton {
            objectName: "generatedOpen"
            kind: "text"
            enabled: sheet.opened
            text: qsTr("Open the key")
            onClicked: {
                const k = sheet.key
                sheet.close()
                sheet.openRequested(k)
            }
        },
        Item {
            Layout.fillWidth: true
        },
        ActionButton {
            objectName: "generatedDone"
            kind: "primary"
            enabled: sheet.opened
            text: qsTr("Done")
            onClicked: sheet.close()
        }
    ]
}
