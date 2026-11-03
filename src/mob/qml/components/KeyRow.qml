// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A key in the list: who it belongs to, then how to reach them; a mark for a
// key of your own, and a word only when something is wrong with it. The key
// ID and fingerprint are on the key's own page.
ItemDelegate {
    id: row

    required property string name
    required property string email
    required property string keyId
    required property bool hasSecret
    required property bool usable
    required property bool expired
    required property bool revoked
    required property string status
    required property string statusText

    // The name, else the address, else the key ID; and under it the address,
    // or the key ID when the address is already on top or missing.
    readonly property string primaryText: name.length > 0 ? name
                                        : email.length > 0 ? email : keyId
    readonly property string secondaryText: name.length > 0 && email.length > 0
                                            ? email : keyId

    leftPadding: Theme.gap
    rightPadding: Theme.gap
    topPadding: 8
    bottomPadding: 8
    implicitHeight: Math.max(Theme.rowHeightTwoLine,
                             implicitContentHeight + topPadding + bottomPadding)
    Accessible.name: primaryText
    Accessible.description: [secondaryText, hasSecret ? qsTr("Your key") : "",
                             status !== "ok" ? statusText : ""]
                            .filter(t => t.length > 0).join(", ")

    contentItem: RowLayout {
        spacing: Theme.gap

        KeyAvatar {
            name: row.primaryText
            usable: row.usable
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: row.primaryText
                elide: Text.ElideRight
                font.pixelSize: Theme.bodySize
                opacity: row.usable ? 1 : 0.6
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Chip {
                    objectName: "keyStatus"
                    visible: row.status !== "ok"
                    text: row.statusText
                    tone: row.status === "expiring_soon" ? Theme.warning
                                                         : Theme.danger
                }

                Label {
                    Layout.fillWidth: true
                    text: row.secondaryText
                    elide: Text.ElideRight
                    font.pixelSize: Theme.secondarySize
                    color: Material.secondaryTextColor
                }
            }
        }

        Icon {
            objectName: "ownKey"
            visible: row.hasSecret
            symbol: "key"
            color: Material.secondaryTextColor
            Accessible.role: Accessible.Graphic
            Accessible.name: qsTr("Your key")
        }
    }
}
