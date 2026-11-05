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

        // The style's checking row, its mark at the end. What is chosen is
        // the sheet's (picked): the row shows it, a tap asks the sheet.
        delegate: CheckDelegate {
            id: choice

            required property string name
            required property string email
            required property string keyId
            required property string secretState
            required property string capabilities

            readonly property bool fixed: sheet.alreadySet[keyId] === true

            objectName: "keyPick_" + keyId
            width: ListView.view.width
            leftPadding: Theme.sheetPad
            enabled: !fixed
            // A real check box, as TalkBack needs one to say "checked": a
            // tap ticks it, and the sheet takes the tick. The binding below
            // stays (Qt 6.10, tst_selection), so the row never shows more
            // or less than the sheet holds.
            checked: fixed || sheet.picked[keyId] === true
            onToggled: sheet.toggle(keyId)
            // Read out, and what puts the style's mark at the end (without
            // words it centres it); the face below shows them.
            text: Wording.keyTitle(name, email, keyId)
            Accessible.name: text
            // What the face shows under the name, which is not read out on
            // its own.
            Accessible.description: [email, keyId, fixed ? qsTr("Already in") : ""]
                                    .filter(t => t.length > 0).join(", ")

            contentItem: KeyChoiceFace {
                idBelow: true
                name: choice.name
                email: choice.email
                keyId: choice.keyId
                secretState: choice.secretState
                capabilities: choice.capabilities
                note: choice.fixed ? qsTr("Already in") : ""

                // The mark's room, as the style's own label keeps it.
                Item {
                    Layout.preferredWidth: choice.indicator.width + choice.spacing
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
