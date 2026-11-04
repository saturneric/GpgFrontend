// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One key group, as the desktop manages it: the keys in it, taken out one
// by one, many put in at once with the key picker; renamed or deleted from
// the menu. Encrypting to it encrypts to every key it holds.
Page {
    id: page

    // The group's id (KeyRoleModel's fingerprint of it).
    property string groupId: ""
    // Opened just after it was made: ask for its keys at once.
    property bool pickOnOpen: false

    signal back()

    readonly property alias notice: done
    readonly property alias picker: picker
    readonly property alias groups: groups

    // Read again on every change of the key list.
    readonly property var group: groups.revision >= 0 ? groups.group(groupId) : ({})
    readonly property var members: groups.revision >= 0 ? groups.members(groupId) : []

    function addKeys() {
        picker.begin(qsTr("Add keys to %1").arg(group.name || ""),
                     groups.memberIds(groupId))
    }

    Component.onCompleted: if (pickOnOpen) Qt.callLater(page.addKeys)

    header: AppBar {
        title: page.group.name || ""
        canGoBack: true
        raised: list.contentY > list.originY
        onBack: page.back()

        BarButton {
            objectName: "groupAddKeys"
            text: qsTr("Add keys")
            icon.source: Theme.icon("add")
            onClicked: page.addKeys()
        }

        OverflowButton {
            objectName: "groupMenu"

            MenuItem {
                objectName: "groupRename"
                text: qsTr("Rename")
                onTriggered: namer.begin(page.group)
            }
            MenuItem {
                objectName: "groupDelete"
                text: qsTr("Delete")
                onTriggered: confirmDelete.open()
            }
        }
    }

    KeyGroupsController {
        id: groups
    }

    ListView {
        id: list

        objectName: "groupMembers"
        anchors.fill: parent
        clip: true
        ScrollIndicator.vertical: ScrollIndicator {}
        model: page.members

        header: Label {
            width: ListView.view.width
            padding: Theme.gap
            wrapMode: Text.Wrap
            color: Material.secondaryTextColor
            font.pixelSize: Theme.secondarySize
            visible: list.count > 0
            height: visible ? implicitHeight : 0
            text: qsTr("%1. Encrypting to this group encrypts to every key in it.")
                  .arg(Wording.memberCount(list.count))
        }

        delegate: ListRow {
            id: member

            required property var modelData

            width: ListView.view.width
            inert: true
            image: modelData.isGroup ? "qrc:/icons/key-group.png" : ""
            symbol: modelData.isGroup ? "" : modelData.missing ? "warning" : "key"
            symbolColor: modelData.missing ? Theme.warning : Material.secondaryTextColor
            text: modelData.name || modelData.email || modelData.id
            detail: modelData.missing ? qsTr("No longer in your keys")
                  : modelData.isGroup ? qsTr("Key group")
                  : [modelData.email || "", (modelData.keyId || "").slice(-8)]
                    .filter(t => t.length > 0).join(" · ")

            BarButton {
                objectName: "groupTakeOut_" + member.modelData.id
                text: qsTr("Take %1 out").arg(member.text)
                icon.source: Theme.icon("close")
                onClicked: {
                    if (groups.removeMember(page.groupId, member.modelData.id))
                        done.show(qsTr("%1 is out of %2.").arg(member.text)
                                  .arg(page.group.name))
                }
            }
        }

        EmptyState {
            objectName: "groupEmpty"
            parent: list
            anchors.centerIn: list
            width: list.width - 64
            visible: list.count === 0
            image: "key_generate"
            text: qsTr("No keys in it yet")
            detail: qsTr("Encrypting to a key group encrypts to every key in "
                         + "it. Add the keys, many at once.")

            Button {
                text: qsTr("Add keys")
                highlighted: true
                onClicked: page.addKeys()
            }
        }
    }

    KeyPickerSheet {
        id: picker
        actionText: qsTr("Add")
        onAccepted: function(keyIds) {
            const added = groups.addMembers(page.groupId, keyIds)
            done.show(qsTr("%n key(s) added.", "", added))
        }
    }

    KeyGroupSheet {
        id: namer
        groups: groups
    }

    MessageAlert {
        id: confirmDelete

        title: qsTr("Delete “%1”?").arg(page.group.name || "")
        message: qsTr("Only the group goes: its keys stay in the key list.")
        acceptText: qsTr("Delete")
        onAccepted: {
            groups.remove(page.groupId)
            page.back()
        }
    }

    Snackbar {
        id: done
    }
}
