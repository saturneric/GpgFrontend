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

    // What is ticked, in one line, and a way to start again.
    RowLayout {
        objectName: "keyPickerSummary"
        Layout.fillWidth: true
        visible: sheet.pickedCount > 0
        spacing: 8

        Icon {
            symbol: "check_circle"
            color: Material.accentColor
            sourceSize: Qt.size(20, 20)
        }

        Label {
            Layout.fillWidth: true
            text: Wording.selectedCount(sheet.pickedCount)
            elide: Text.ElideRight
            font.pixelSize: Theme.secondarySize
            font.weight: Font.Medium
        }

        ActionButton {
            kind: "text"
            text: qsTr("Clear")
            onClicked: sheet.picked = {}
        }
    }

    KeyFilterModel {
        id: filtered
        sourceModel: KeyListController.keys
        searchText: search.text
        category: sheet.category
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

        delegate: ItemDelegate {
            id: choice

            required property string name
            required property string email
            required property string keyId
            required property string secretState
            required property string capabilities

            readonly property bool fixed: sheet.alreadySet[keyId] === true
            readonly property bool chosen: fixed || sheet.picked[keyId] === true

            objectName: "keyPick_" + keyId
            width: ListView.view.width
            leftPadding: Theme.sheetPad
            rightPadding: 8
            enabled: !fixed
            onClicked: sheet.toggle(keyId)
            Accessible.role: Accessible.CheckBox
            Accessible.checked: chosen
            Accessible.name: Wording.keyTitle(name, email, keyId)

            contentItem: KeyChoiceFace {
                idBelow: true
                name: choice.name
                email: choice.email
                keyId: choice.keyId
                secretState: choice.secretState
                capabilities: choice.capabilities
                note: choice.fixed ? qsTr("Already in") : ""

                // The mark of the choice; a tap on it is a tap on the row.
                CheckBox {
                    checkable: false
                    checked: choice.chosen
                    enabled: !choice.fixed
                    focusPolicy: Qt.NoFocus
                    onClicked: sheet.toggle(choice.keyId)
                    Accessible.ignored: true
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        visible: filtered.count === 0
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: qsTr("No key matches.")
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
