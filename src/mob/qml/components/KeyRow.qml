// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A key in a list that manages keys: the face every key list shows
// (KeyChoiceFace). A tap opens the key; a long press, its categories. It
// chooses nothing for an operation: that is the key pickers' (KeyToolbox).
ItemDelegate {
    id: row

    required property string name
    required property string email
    required property string keyId
    required property bool usable
    required property string status
    required property string statusText
    required property string fingerprint
    required property string secretState
    required property string capabilities
    // False for a key group, which has members instead of an address.
    required property bool isKey
    required property var members

    // A long press: which categories the key is in.
    signal categoriesRequested()

    // A control at the end (a button), if any.
    default property alias trailing: face.data

    readonly property string primaryText: Wording.keyTitle(name, email, keyId)

    leftPadding: Theme.gap
    rightPadding: Theme.gap
    topPadding: 8
    bottomPadding: 8
    // At least a two-line row; more when the text needs it.
    implicitHeight: Math.max(Theme.touch + 8,
                             implicitContentHeight + topPadding + bottomPadding)
    Accessible.name: primaryText
    Accessible.description: [!isKey ? qsTr("Key group") : "",
                             Wording.keyTagDescription(secretState),
                             Wording.capabilityWords(capabilities),
                             email, keyId,
                             status !== "ok" ? statusText : ""]
                            .filter(t => t.length > 0).join(", ")
    onPressAndHold: categoriesRequested()

    contentItem: KeyChoiceFace {
        id: face

        name: row.name
        email: row.email
        keyId: row.keyId
        isKey: row.isKey
        usable: row.usable
        members: row.members
        secretState: row.secretState
        capabilities: row.capabilities
        status: row.status
        statusText: row.statusText
    }
}
