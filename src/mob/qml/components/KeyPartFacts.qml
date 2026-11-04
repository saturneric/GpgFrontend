// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The facts of one part of a key, the primary key or a subkey, as the key's
// page and the subkey sheet both show them. A fact without a value is left
// out, so a part shows only what it has.
GridLayout {
    id: facts

    // The part, by the names the key details give it: keyId, algorithm,
    // algorithmDetail, keySize, usage, created.
    property var part: ({})
    // What kind of part it is ("Type", "Key format"), and what.
    property string kindTitle: ""
    property string kindValue: ""
    property string expiresText: ""
    property color expiresColor: Material.foreground
    property string lastUpdate: ""
    property string secretText: ""
    property string cardText: ""

    columns: Theme.fontScale >= 1.3 ? 1 : 2
    columnSpacing: Theme.gap
    rowSpacing: 8

    Fact {
        title: qsTr("Key ID")
        value: facts.part.keyId || ""
        mono: true
        wide: true
    }
    Fact {
        title: facts.kindTitle
        value: facts.kindValue
    }
    Fact {
        title: qsTr("Algorithm")
        value: facts.part.algorithm || ""
    }
    Fact {
        title: qsTr("Algorithm detail")
        value: facts.part.algorithmDetail || ""
    }
    Fact {
        title: qsTr("Key size")
        value: facts.part.keySize > 0 ? qsTr("%1 bits").arg(facts.part.keySize) : ""
    }
    Fact {
        title: qsTr("Usage")
        value: Wording.usage(facts.part.usage)
    }
    Fact {
        title: qsTr("Created")
        value: Theme.date(facts.part.created)
    }
    Fact {
        title: qsTr("Expires")
        value: facts.expiresText
        valueColor: facts.expiresColor
    }
    Fact {
        title: qsTr("Last update")
        value: facts.lastUpdate
    }
    Fact {
        title: qsTr("Secret part")
        value: facts.secretText
    }
    Fact {
        title: qsTr("Smart card")
        value: facts.cardText
    }
}
