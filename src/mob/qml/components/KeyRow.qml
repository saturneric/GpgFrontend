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
    required property string fingerprint
    // False for a key group, which has members instead of an address.
    required property bool isKey
    required property var members

    // Whether the key is one the work is encrypted to (WorkSelection).
    readonly property bool recipient: WorkSelection.revision >= 0
                                      && WorkSelection.isRecipient(fingerprint)

    // A tap on the face: one more recipient, or one fewer.
    signal recipientToggled()
    // A long press: which categories the key is in.
    signal categoriesRequested()

    // A control at the end (a button), if any, after the own-key mark.
    default property alias trailing: trailingRow.data

    // What every key list says of a key (Wording.keyTitle, keySubtitle).
    readonly property string primaryText: Wording.keyTitle(name, email, keyId)
    readonly property string secondaryText: !isKey ? Wording.memberCount((members || []).length)
                                            : Wording.keySubtitle(name, email, keyId)

    leftPadding: Theme.gap
    rightPadding: Theme.gap
    topPadding: 8
    bottomPadding: 8
    implicitHeight: Math.max(Theme.rowHeightTwoLine,
                             implicitContentHeight + topPadding + bottomPadding)
    Accessible.name: primaryText
    Accessible.description: [!isKey ? qsTr("Key group") : "", secondaryText,
                             hasSecret ? qsTr("Your key") : "",
                             status !== "ok" ? statusText : "",
                             recipient ? qsTr("Chosen to encrypt to") : ""]
                            .filter(t => t.length > 0).join(", ")
    onPressAndHold: categoriesRequested()

    contentItem: RowLayout {
        spacing: Theme.gap

        // The face, or a check while it is chosen to encrypt to.
        Item {
            implicitWidth: 40
            implicitHeight: 40

            KeyAvatar {
                anchors.centerIn: parent
                visible: !row.recipient && row.isKey
                name: row.primaryText
                usable: row.usable
            }

            // A key group: the desktop's mark for one.
            Rectangle {
                anchors.fill: parent
                visible: !row.recipient && !row.isKey
                radius: width / 2
                color: Theme.alpha(Material.accentColor, 0.12)

                Image {
                    objectName: "groupMark"
                    anchors.centerIn: parent
                    source: "qrc:/icons/key-group.png"
                    sourceSize: Qt.size(24, 24)
                    opacity: row.usable ? 1 : 0.5
                }
            }

            Rectangle {
                anchors.fill: parent
                visible: row.recipient
                radius: width / 2
                color: Material.accentColor

                Icon {
                    anchors.centerIn: parent
                    symbol: "check_box"
                    color: "white"
                }
            }

            // Takes the press, so the row under it does not open as well.
            MouseArea {
                anchors.fill: parent
                onClicked: row.recipientToggled()
            }

            Accessible.role: Accessible.CheckBox
            Accessible.checked: row.recipient
            Accessible.name: row.recipient ? qsTr("Do not encrypt to %1").arg(row.primaryText)
                                           : qsTr("Encrypt to %1").arg(row.primaryText)
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

        RowLayout {
            id: trailingRow
            spacing: 0
            visible: children.length > 0
        }
    }
}
