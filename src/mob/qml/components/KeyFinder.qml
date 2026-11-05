// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GpgFrontend.Mobile

// Finding a key, as every key list does it (the Keys page, the key toolbox,
// the key picker): a search on what a key says, and beside it the colour of
// the category shown, which opens the list of categories to choose another.
RowLayout {
    id: finder

    // The categories offered, in the order the list shows them:
    // {"id", "name", "color"}.
    property var entries: []
    // The category shown; "" for all keys. Kept by whoever places it, which
    // hears categoryChosen().
    property string category: ""
    // The list of categories also makes and manages them (the Keys page).
    property alias manageable: list.manageable
    // Names for tests and for finding them.
    property alias searchName: search.objectName
    property alias categoryName: button.objectName
    // The search field, to read, clear and focus.
    readonly property alias field: search

    readonly property var currentCategory: {
        const found = entries.find(c => c.id === finder.category)
        return found || {"id": "", "name": qsTr("All keys"), "color": ""}
    }

    signal categoryChosen(string id)
    signal newRequested()
    signal manageRequested()

    objectName: "keyFinder"
    spacing: Theme.gapSmall

    // The search takes all the room the small category button leaves; both
    // drawn a little lower than a touch target, which they still are.
    SearchField {
        id: search

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        pillHeight: 44
        hint: qsTr("Search name, email or key ID")
    }

    CategoryButton {
        id: button

        // As tall as the search (larger with large text); not by filling,
        // which would make the whole row fill the page.
        Layout.preferredHeight: search.height
        pillHeight: 44
        category: finder.currentCategory
        onClicked: list.open()
    }

    CategoryListSheet {
        id: list

        entries: finder.entries
        current: finder.category
        manageable: false
        onChosen: category => finder.categoryChosen(category.id)
        onNewRequested: finder.newRequested()
        onManageRequested: finder.manageRequested()
    }
}
