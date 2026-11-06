// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Many keys chosen at once, the way the key toolbox chooses them: searched,
// narrowed by category, ticked, the list filling the sheet. Keys already
// where they are going are shown so, and not offered again. What is ticked
// is its own (never the operation's keys), said in one line.
BottomSheet {
    id: sheet

    // Key IDs that are already in (a category's), shown ticked and fixed.
    property var already: []
    // The same, to look up from each row.
    readonly property var alreadySet: {
        const set = {}
        for (const id of already) set[id] = true
        return set
    }
    // What the button does with what is ticked: "Add %n keys".
    property string actionText: qsTr("Add")
    // Ticked so far: key ID -> true.
    property var picked: ({})
    readonly property int pickedCount: Object.keys(picked).length
    // The key category shown; "" for all keys.
    property string category: ""
    // Only keys that can encrypt are offered: what a key group takes.
    property bool onlyEncryptCapable: false

    readonly property var categoryEntries: categoryStore.categories.concat(categoryStore.builtins)
    // The search field (KeyFinder's).
    readonly property alias search: finder.field

    // The ticked key IDs.
    signal accepted(var keyIds)

    function begin(title, alreadyIn) {
        sheet.title = title
        already = alreadyIn || []
        picked = {}
        category = ""
        search.clear()
        // On the list, not in a search left with the focus last time (the
        // keyboard came up by itself).
        search.focus = false
        open()
    }

    function toggle(keyId) {
        const next = Object.assign({}, picked)
        if (next[keyId]) delete next[keyId]
        else next[keyId] = true
        picked = next
    }

    objectName: "keyPicker"
    expandable: true
    listMode: true
    stepBack: function() {
        if (search.text.length > 0) {
            search.clear()
            return true
        }
        return false
    }

    KeyCategoriesController {
        id: categoryStore
    }

    KeyFinder {
        id: finder

        Layout.fillWidth: true
        searchName: "keyPickerSearch"
        categoryName: "keyPickerCategory"
        entries: sheet.categoryEntries
        category: sheet.category
        onCategoryChosen: id => sheet.category = id
    }

    // What is ticked, in one row that is always there and always as tall,
    // as the key toolbox says it, and a way to start again.
    KeyChoiceSummary {
        objectName: "keyPickerSummary"
        enabled: false
        text: sheet.pickedCount > 0 ? Wording.selectedCount(sheet.pickedCount)
                                    : qsTranslate("KeyToolbox", "No keys chosen yet")
        symbol: sheet.pickedCount > 0 ? "check_circle" : ""

        ActionButton {
            objectName: "keyPickerClear"
            kind: "text"
            enabled: sheet.pickedCount > 0
            text: qsTr("Clear")
            onClicked: sheet.picked = {}
        }
    }

    KeyFilterModel {
        id: filtered
        sourceModel: KeyListController.keys
        searchText: search.text
        category: sheet.category
        onlyEncryptCapable: sheet.onlyEncryptCapable
    }

    // Only the rows in view are made: there may be a thousand keys.
    ListView {
        id: list

        objectName: "keyPickerList"
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        Layout.fillHeight: true
        Layout.preferredHeight: Math.max(contentHeight, Theme.touch * 2) + bottomMargin
        Layout.minimumHeight: Theme.touch * 2
        bottomMargin: Theme.gapSmall
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        reuseItems: true
        model: filtered
        ScrollIndicator.vertical: ScrollIndicator {}

        // The same row as the key toolbox's. What is chosen is the
        // sheet's (picked): the row shows it, a tap asks the sheet.
        delegate: KeyChoiceRow {
            readonly property bool fixed: sheet.alreadySet[keyId] === true

            objectName: "keyPick_" + keyId
            enabled: !fixed
            chosen: fixed || sheet.picked[keyId] === true
            note: fixed ? qsTr("Already in") : ""
            Accessible.description: [email, keyId, note]
                                    .filter(t => t.length > 0).join(", ")
            onChooseRequested: sheet.toggle(keyId)
        }

        // Over the list, as the toolbox says it: nothing moves when it
        // shows.
        Label {
            objectName: "keyPickerEmpty"
            anchors.fill: parent
            anchors.margins: Theme.gap
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            visible: filtered.count === 0
            color: Material.secondaryTextColor
            text: qsTr("No key matches.")
        }
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        ActionButton {
            kind: "text"
            text: qsTr("Cancel")
            onClicked: sheet.close()
        },
        ActionButton {
            objectName: "keyPickerAccept"
            text: sheet.pickedCount > 0
                  ? qsTr("%1 (%2)").arg(sheet.actionText).arg(sheet.pickedCount)
                  : sheet.actionText
            enabled: sheet.pickedCount > 0
            onClicked: {
                const ids = Object.keys(sheet.picked)
                sheet.close()
                sheet.accepted(ids)
            }
        }
    ]
}
