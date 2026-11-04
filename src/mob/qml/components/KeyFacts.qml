// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// What a key is, as a person checks it: its state in a line, the whole
// fingerprint in groups of four that wrap as the width and the text size
// need (and copied whole), then the rest. Dates as 2026-11-03, read one way
// everywhere.
ColumnLayout {
    id: facts

    // The key list's roles of the key, by name.
    property var key: ({})

    readonly property string state: {
        if (key.revoked === true || key.status === "revoked") return qsTr("Revoked")
        if (key.status === "disabled") return qsTr("Disabled")
        if (key.expired === true || key.status === "expired") {
            const when = Theme.date(key.expires)
            return when.length > 0 ? qsTr("Expired on %1").arg(when) : qsTr("Expired")
        }
        const until = Theme.date(key.expires)
        return until.length > 0 ? qsTr("Valid until %1").arg(until)
                                : qsTr("Valid, does not expire")
    }

    spacing: 12

    Label {
        objectName: "keyState"
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        text: facts.state
        color: (facts.key.status || "ok") === "ok" ? Material.foreground
               : facts.key.status === "expiring_soon" ? Theme.warning
                                                      : Theme.danger
    }

    FingerprintView {
        Layout.fillWidth: true
        fingerprint: facts.key.fingerprint || ""
    }

    // As the key's page lists a part of a key (KeyPartFacts), with what the
    // list knows: what the key can do stands for its usage.
    KeyPartFacts {
        Layout.fillWidth: true
        part: facts.key
        kindTitle: qsTr("Can")
        kindValue: [facts.key.canEncrypt === true ? qsTr("encrypt") : "",
                    facts.key.canSign === true ? qsTr("sign") : ""]
                   .filter(t => t.length > 0).join(", ")
        expiresText: facts.key.expires && !isNaN(facts.key.expires)
                     ? Theme.date(facts.key.expires) : qsTr("Never")
        secretText: facts.key.hasSecret === true ? qsTr("Yours, on this device")
                                                 : qsTr("Not here")
    }
}
