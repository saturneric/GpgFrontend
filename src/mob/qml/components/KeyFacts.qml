// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What a key is, as a person checks it: its state in a line, the whole
// fingerprint in groups of four that wrap as the width and the text size
// need (and copied whole), then the rest as label and value, one per line.
// Dates as 2026-11-03, read one way everywhere.
//
// What the key can do is the whole key's; each part -- the primary key and
// every subkey -- says its own algorithm and what it does itself, so an
// Ed25519 primary key that signs never seems to be what encrypts (that is
// its Curve25519 subkey).
ColumnLayout {
    id: facts

    // The key list's roles of the key, by name.
    property var key: ({})
    // Its parts, as KeyActionsController.describe() lists them (subkeys,
    // the primary first); none known yet: the primary alone, from the list.
    property var parts: []

    // Worded as everywhere else a key's state is (Wording.keyState).
    readonly property string state: Wording.keyState(key)

    readonly property var shownParts: parts.length > 0 ? parts
        : [{"primary": true, "algorithm": key.algorithm || "", "capabilities": ""}]

    spacing: Theme.gapSmall

    // Only what must be seen at once -- expired, revoked, disabled: the
    // expiration date itself is a fact below, said once.
    Label {
        objectName: "keyState"
        Layout.fillWidth: true
        visible: ["expired", "revoked", "disabled"].includes(facts.key.status || "ok")
        wrapMode: Text.Wrap
        text: facts.state
        color: Theme.danger
    }

    FingerprintView {
        Layout.fillWidth: true
        fingerprint: facts.key.fingerprint || ""
    }

    InfoRow {
        Layout.fillWidth: true
        label: qsTr("Key ID")
        value: facts.key.keyId || ""
        mono: true
    }

    InfoRow {
        objectName: "keyCan"
        Layout.fillWidth: true
        label: qsTranslate("KeyPartFacts", "Usage")
        value: Wording.capabilityWords(facts.key.capabilities)
    }

    // Each part, with what it is and what it does itself.
    Repeater {
        model: facts.shownParts

        InfoRow {
            required property var modelData

            objectName: "keyPart"
            Layout.fillWidth: true
            label: modelData.primary === true ? qsTr("Primary key") : qsTr("Subkey")
            value: [modelData.algorithmDetail || modelData.algorithm || "",
                    modelData.capabilities || "",
                    modelData.revoked === true || modelData.expired === true
                    || modelData.disabled === true ? Wording.keyState(modelData) : ""]
                   .filter(t => t.length > 0).join(" · ")
        }
    }

    InfoRow {
        Layout.fillWidth: true
        label: qsTr("Created")
        value: Theme.date(facts.key.created)
    }

    InfoRow {
        Layout.fillWidth: true
        label: qsTr("Expires")
        value: facts.key.expires && !isNaN(facts.key.expires)
               ? Theme.date(facts.key.expires) : qsTr("Never")
        valueColor: facts.key.expired === true ? Theme.danger : Material.foreground
    }

    InfoRow {
        Layout.fillWidth: true
        label: qsTr("Secret part")
        value: facts.key.hasSecret === true ? qsTr("On this device") : qsTr("Not here")
    }
}
