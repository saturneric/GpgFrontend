// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// A key as every list shows it -- the Keys page, the key pickers, a group's
// members, a new key -- the same everywhere:
//  - who it is (the name, else the address), what it can do in GnuPG's
//    letters (CESA: certify, encrypt, sign, authenticate) on a tint of
//    their own, and GnuPG's mark for what of it is here (pub, sec, sec#,
//    sec>), always last, in the accent;
//  - the address, and the key ID always in the same place at the right, in
//    the desktop's monospace, never pushed out by a long address;
//  - only when there is something to say: what is wrong with it (expired,
//    revoked) or a note from the list that shows it.
// A key group shows its mark and how many keys it holds. Controls declared
// inside it (a check box, a button) go at the end. Its height is its
// content's: larger text and a long note take more room, never get cut.
RowLayout {
    id: face

    property string name: ""
    property string email: ""
    property string keyId: ""
    property bool isKey: true
    property bool usable: true
    property var members: []
    // KeyRoleModel's secretState: what the mark says.
    property string secretState: ""
    // KeyRoleModel's capabilities: what the key as a whole can do, "" for
    // nothing to say.
    property string capabilities: ""
    // KeyRoleModel's status ("ok" says nothing) and its words.
    property string status: "ok"
    property string statusText: ""
    // A word from the list that shows the key, under the rest; in red when
    // @ref problem.
    property string note: ""
    property bool problem: false

    readonly property string title: Wording.keyTitle(name, email, keyId)
    // The address under the name, when the name is on top.
    readonly property string address: name.length > 0 ? email : ""
    // The key ID on a line of its own, under the address: where controls at
    // the end take the room (the pickers), and with large text.
    property bool idBelow: false
    readonly property bool stacked: idBelow || Theme.fontScale > 1.3

    spacing: 12

    // What it can do: GnuPG's letters in a chip, so they never read as part
    // of the mark beside them; the same chip as on the key's own page.
    component CapabilityPill: Chip {
        property string letters

        text: letters
        font.family: Theme.mono
        tone: Material.secondaryTextColor
        Accessible.role: Accessible.StaticText
        Accessible.name: Wording.capabilityWords(letters)
    }

    KeyAvatar {
        Layout.alignment: Qt.AlignTop
        Layout.topMargin: 2
        visible: face.isKey
        name: face.title
        usable: face.usable
    }

    // A key group: the desktop's mark for one.
    Rectangle {
        Layout.alignment: Qt.AlignTop
        Layout.topMargin: 2
        visible: !face.isKey
        implicitWidth: 40
        implicitHeight: 40
        radius: 20
        color: Theme.alpha(Material.accentColor, 0.12)

        Image {
            objectName: "groupMark"
            anchors.centerIn: parent
            source: "qrc:/icons/key-group.png"
            sourceSize: Qt.size(24, 24)
            opacity: face.usable ? 1 : 0.5
        }
    }

    // Takes what the face and the controls at the end leave, however long
    // its words: they elide or wrap in it, never push the controls out.
    ColumnLayout {
        Layout.fillWidth: true
        Layout.preferredWidth: 0
        Layout.minimumWidth: 0
        spacing: 1

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                objectName: "keyTitle"
                Layout.fillWidth: true
                text: face.title
                elide: Text.ElideRight
                font.pixelSize: Theme.bodySize
                opacity: face.usable ? 1 : 0.6
            }

            // Beside the name while the key ID shares the line under it;
            // beside the key ID where that has a line of its own, leaving
            // the name its room.
            CapabilityPill {
                objectName: "keyCapabilities"
                letters: face.capabilities
                visible: !face.stacked && face.isKey && face.capabilities.length > 0
            }

            Label {
                objectName: "keyTag"
                visible: text.length > 0
                text: Wording.keyTag(face.secretState)
                font.family: Theme.mono
                font.pixelSize: Theme.labelSize
                color: face.secretState === "none" ? Material.secondaryTextColor
                                                   : Material.accentColor
                Accessible.name: Wording.keyTagDescription(face.secretState)
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: face.stacked ? 1 : 2
            columnSpacing: 8
            rowSpacing: 0
            visible: !face.isKey || face.address.length > 0 || face.keyId.length > 0

            Label {
                objectName: "keyAddress"
                Layout.fillWidth: true
                visible: text.length > 0
                text: !face.isKey ? Wording.memberCount((face.members || []).length)
                                  : face.address
                elide: Text.ElideRight
                font.pixelSize: Theme.secondarySize
                color: Material.secondaryTextColor
            }

            // Where the address ends, the key ID begins: a long address is
            // cut short, the key ID never is.
            RowLayout {
                Layout.alignment: face.stacked ? Qt.AlignLeft : Qt.AlignRight
                visible: face.isKey && face.keyId.length > 0
                spacing: 8

                Label {
                    objectName: "keyId"
                    text: face.keyId
                    font.family: Theme.mono
                    font.pixelSize: Theme.labelSize
                    color: Material.secondaryTextColor
                }

                CapabilityPill {
                    objectName: "keyCapabilitiesBelow"
                    letters: face.capabilities
                    visible: face.stacked && face.isKey && face.capabilities.length > 0
                }
            }
        }

        Flow {
            // What it shows, said here rather than read from its children:
            // their visibility follows the Flow's own, so a row made with no
            // note never showed one given to it later ("Already in").
            readonly property bool showsStatus: face.status.length > 0
                                                && face.status !== "ok"
                                                && face.statusText.length > 0
                                                && face.note !== face.statusText

            Layout.fillWidth: true
            Layout.topMargin: 2
            spacing: 8
            visible: showsStatus || face.note.length > 0

            Chip {
                id: chip

                objectName: "keyStatus"
                visible: face.status.length > 0 && face.status !== "ok"
                         && face.statusText.length > 0
                         && face.note !== face.statusText
                text: face.statusText
                tone: face.status === "expiring_soon" ? Theme.warning : Theme.danger
            }

            Label {
                id: noteLabel

                objectName: "keyNote"
                width: parent.width
                visible: face.note.length > 0
                text: face.note
                wrapMode: Text.Wrap
                font.pixelSize: Theme.secondarySize
                color: face.problem ? Theme.danger : Material.secondaryTextColor
            }
        }
    }
}
