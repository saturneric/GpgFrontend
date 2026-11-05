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

    // Not a stop for a screen reader: a page has no name of its own, only
    // its parts do (Qt gives a Page the role of an unnamed tab).
    Accessible.ignored: true

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

        // The last row scrolls clear of the add button.
        bottomMargin: Theme.fabBand

        // A section label; what a group means is a tap on (i) away.
        header: RowLayout {
            width: ListView.view.width
            visible: list.count > 0
            height: visible ? implicitHeight : 0
            spacing: 0

            SectionHeader {
                objectName: "groupMembersLabel"
                Layout.fillWidth: true
                text: qsTr("Member keys (%1)").arg(list.count)
            }

            BarButton {
                objectName: "groupHelp"
                Layout.rightMargin: 4
                text: qsTr("About key groups")
                icon.source: Theme.icon("info")
                onClicked: groupHelp.open()
            }
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

            // Out of the group only: the key itself stays in the key list.
            BarButton {
                objectName: "groupTakeOut_" + member.modelData.id
                text: qsTr("Remove %1 from the group").arg(member.text)
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
            detail: qsTr("Use + to add keys, many at once. Encrypting to a "
                         + "key group encrypts to every key in it.")
        }
    }

    Fab {
        objectName: "addGroupMembers"
        text: qsTr("Add keys to the group")
        icon.source: Theme.icon("add")
        onClicked: page.addKeys()
    }

    MessageAlert {
        id: groupHelp

        objectName: "groupHelpAlert"
        title: qsTr("Key groups")
        message: qsTr("Encrypting to a key group encrypts to every key in it, "
                      + "as if each were chosen. Removing a key from the group "
                      + "keeps the key in your key list.")
    }

    KeyPickerSheet {
        id: picker
        actionText: qsTr("Add")
        onlyEncryptCapable: true
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
        // Above the add button, never over it.
        bottomOffset: Theme.fabBand - 12
    }
}
