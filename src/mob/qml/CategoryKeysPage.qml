// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One key category: the keys in it, taken out one by one, and many put in at
// once with a picker like the key toolbox. Renamed, recoloured or deleted
// from the menu.
MemberListPage {
    id: page

    // The category: {id, name, color}.
    property var category: ({})

    // Its name and colour as they are now, after an edit here.
    readonly property var current: {
        const all = store.categories
        for (const c of all) if (c.id === category.id) return c
        return category
    }

    // A membership changed here or elsewhere: the list follows. (Signals
    // named Signal... have no on-handler in QML.)
    Component.onCompleted: store.SignalChanged.connect(members.refreshCategory)

    function addKeys() {
        picker.begin(qsTr("Add keys to %1").arg(current.name),
                     store.keyIdsOf(category.id))
    }

    title: current.name || ""
    menuName: "categoryMenu"
    menuItems: [
        MenuItem {
            text: qsTr("Edit")
            onTriggered: editor.begin(page.current)
        },
        MenuItem {
            objectName: "categoryDelete"
            text: qsTr("Delete")
            onTriggered: page.confirmDelete.open()
        }
    ]

    listName: "categoryKeys"
    model: KeyFilterModel {
        id: members
        sourceModel: KeyListController.keys
        category: page.category.id || ""
        // The key groups put in it too.
        withGroups: true
    }
    delegate: KeyRow {
        id: member

        required property var model

        width: ListView.view.width
        onClicked: page.openMember(!model.isKey, model.fingerprint, model.fingerprint)

        BarButton {
            objectName: "categoryTakeOut_" + member.keyId
            text: qsTr("Remove %1 from the category").arg(member.primaryText)
            icon.source: Theme.icon("close")
            onClicked: {
                const category = page.category.id
                const key = member.keyId
                page.tookOut(store.setMember(category, key, false),
                             qsTr("%1 is out of %2.").arg(member.primaryText)
                             .arg(page.current.name),
                             qsTr("%1 could not be taken out of the category.")
                             .arg(member.primaryText),
                             () => store.setMember(category, key, true))
            }
        }
    }

    emptyName: "categoryEmpty"
    emptyText: qsTr("No keys in it yet")
    emptyDetail: qsTr("Use + to add keys you have, many at once, or "
                      + "long-press a key in the key list.")

    // Adding keys one has to the category: never making a new key.
    addName: "categoryAddKeys"
    addText: qsTr("Add keys to the category")
    onAddRequested: addKeys()

    onKeysPicked: function(keyIds) {
        const added = store.setMembers(page.category.id, keyIds, true)
        notice.show(qsTr("%n key(s) added.", "", added))
    }

    deleteTitle: qsTr("Delete “%1”?").arg(current.name || "")
    deleteMessage: qsTr("Only the category goes: its keys stay, in the key list "
                        + "and in their other categories.")
    confirmDelete.acceptText: qsTr("Delete")
    onDeleteAccepted: {
        store.remove(page.category.id)
        page.back()
    }

    KeyCategoriesController {
        id: store
    }

    CategorySheet {
        id: editor
        categories: store
    }
}
