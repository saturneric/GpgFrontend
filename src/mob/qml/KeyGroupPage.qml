// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One key group, as the desktop manages it: the keys in it, taken out one
// by one, many put in at once with the key picker; renamed or deleted from
// the menu. Encrypting to it encrypts to every key it holds.
MemberListPage {
    id: page

    // The group's id (KeyRoleModel's fingerprint of it).
    property string groupId: ""
    // Opened just after it was made: ask for its keys at once.
    property bool pickOnOpen: false

    readonly property alias groups: groups

    // Read again on every change of the key list.
    readonly property var group: groups.revision >= 0 ? groups.group(groupId) : ({})
    readonly property var members: groups.revision >= 0 ? groups.members(groupId) : []

    function addKeys() {
        picker.begin(qsTr("Add keys to %1").arg(group.name || ""),
                     groups.memberIds(groupId))
    }

    Component.onCompleted: if (pickOnOpen) Qt.callLater(page.addKeys)

    title: group.name || ""
    menuName: "groupMenu"
    menuItems: [
        MenuItem {
            objectName: "groupEdit"
            text: qsTr("Edit")
            onTriggered: namer.begin(page.group)
        },
        MenuItem {
            objectName: "groupCategories"
            text: qsTr("Categories")
            onTriggered: groupCategories.begin({
                "keyId": page.groupId, "name": page.group.name || ""
            })
        },
        MenuItem {
            objectName: "groupDelete"
            text: qsTr("Delete")
            onTriggered: page.confirmDelete.open()
        }
    ]

    listName: "groupMembers"
    model: members

    // The group's email and comment, as the desktop keeps them, when it has
    // any; then a section label, what a group means a tap on (i) away.
    listHeader: ColumnLayout {
        width: ListView.view.width
        spacing: 0

        ColumnLayout {
            objectName: "groupDetails"
            Layout.fillWidth: true
            Layout.leftMargin: Theme.gap
            Layout.rightMargin: Theme.gap
            Layout.topMargin: Theme.gapSmall
            visible: (page.group.email || "").length > 0
                     || (page.group.comment || "").length > 0
            spacing: Theme.gapSmall

            InfoRow {
                objectName: "groupEmail"
                Layout.fillWidth: true
                label: qsTr("Email")
                value: page.group.email || ""
            }

            InfoRow {
                objectName: "groupComment"
                Layout.fillWidth: true
                label: qsTr("Comment")
                value: page.group.comment || ""
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: page.count > 0
            spacing: 0

            SectionHeader {
                objectName: "groupMembersLabel"
                Layout.fillWidth: true
                text: qsTr("Member keys (%1)").arg(page.count)
            }

            BarButton {
                objectName: "groupHelp"
                Layout.rightMargin: 4
                text: qsTr("About key groups")
                icon.source: Theme.icon("info")
                onClicked: groupHelp.open()
            }
        }
    }

    delegate: ListRow {
        id: member

        required property var modelData

        width: ListView.view.width
        // One gone from the keys has nothing to open.
        inert: modelData.missing === true
        onClicked: page.openMember(modelData.isGroup === true, modelData.id,
                                   modelData.fingerprint)
        symbol: modelData.isGroup ? "group" : modelData.missing ? "warning" : "key"
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
                const group = page.groupId
                const key = member.modelData.id
                page.tookOut(groups.removeMember(group, key),
                             qsTr("%1 is out of %2.").arg(member.text)
                             .arg(page.group.name),
                             qsTr("%1 could not be taken out of the group.")
                             .arg(member.text),
                             () => groups.addMembers(group, [key]))
            }
        }
    }

    emptyName: "groupEmpty"
    emptyText: qsTr("No keys in it yet")
    emptyDetail: qsTr("Use + to add keys, many at once. Encrypting to a "
                      + "key group encrypts to every key in it.")

    addName: "addGroupMembers"
    addText: qsTr("Add keys to the group")
    onAddRequested: addKeys()

    picker.onlyEncryptCapable: true
    onKeysPicked: function(keyIds) {
        const added = groups.addMembers(page.groupId, keyIds)
        notice.show(qsTr("%n key(s) added.", "", added))
    }

    deleteTitle: qsTr("Delete “%1”?").arg(group.name || "")
    // The groups it is a member of lose it too: named, as the desktop does.
    readonly property var parentNames: groups.revision >= 0 ? groups.parentNames(groupId) : []
    deleteMessage: qsTr("Only the group goes: its keys stay in the key list.")
                   + (parentNames.length > 0
                      ? "\n\n" + qsTr("It is also in %1, which will lose it.")
                        .arg(parentNames.join(", "))
                      : "")
    confirmDelete.acceptText: qsTr("Delete")
    // Gone, then the page with it; not gone, said, and the page stays.
    onDeleteAccepted: {
        if (groups.remove(page.groupId)) page.back()
        else notice.show(qsTr("The group could not be deleted."))
    }

    KeyGroupsController {
        id: groups
    }

    // Which of the person's categories the group is in, as for a key.
    KeyCategoriesController {
        id: categoryStore
    }

    KeyCategoriesSheet {
        id: groupCategories
        categories: categoryStore
    }

    MessageAlert {
        id: groupHelp

        objectName: "groupHelpAlert"
        title: qsTr("Key groups")
        message: qsTr("Encrypting to a key group encrypts to every key in it, "
                      + "as if each were chosen. Removing a key from the group "
                      + "keeps the key in your key list.")
    }

    KeyGroupSheet {
        id: namer
        groups: groups
    }
}
