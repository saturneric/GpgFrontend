// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Many keys chosen at once, the way the key toolbox chooses them: searched,
// narrowed, ticked. Keys already where they are going are shown so, and not
// offered again.
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
    // "all", "mine" or "others".
    property string filter: "all"

    // The ticked key IDs.
    signal accepted(var keyIds)

    function begin(title, alreadyIn) {
        sheet.title = title
        already = alreadyIn || []
        picked = {}
        filter = "all"
        search.clear()
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
    stepBack: function() {
        if (search.text.length > 0) {
            search.clear()
            return true
        }
        return false
    }

    SearchField {
        id: search
        objectName: "keyPickerSearch"
        Layout.fillWidth: true
        hint: qsTr("Search name, email or key ID")
    }

    Row {
        spacing: 8

        Repeater {
            model: [{"id": "all", "name": qsTr("All")},
                    {"id": "mine", "name": qsTr("Mine")},
                    {"id": "others", "name": qsTr("Others'")}]

            Button {
                required property var modelData
                text: modelData.name
                flat: sheet.filter !== modelData.id
                highlighted: sheet.filter === modelData.id
                font.pixelSize: Theme.secondarySize
                topInset: 0
                bottomInset: 0
                implicitHeight: Math.max(36, Theme.secondarySize * 2.4)
                onClicked: sheet.filter = modelData.id
            }
        }
    }

    KeyFilterModel {
        id: filtered
        sourceModel: KeyListController.keys
        searchText: search.text
        category: sheet.filter === "mine" ? "has_private_key"
                : sheet.filter === "others" ? "only_public_key" : ""
    }

    // Only the rows in view are made: there may be a thousand keys.
    ListView {
        id: list

        objectName: "keyPickerList"
        Layout.fillWidth: true
        Layout.leftMargin: -24
        Layout.rightMargin: -24
        Layout.preferredHeight: Math.min(
            contentHeight,
            // What the keyboard leaves: with it up the list is shorter, so
            // the field typed into stays in view.
            Math.max(Theme.rowHeightTwoLine * 2,
                     ((Window.height || 800) - Theme.keyboard) * 0.4))
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

            readonly property bool fixed: sheet.alreadySet[keyId] === true
            readonly property bool chosen: fixed || sheet.picked[keyId] === true

            objectName: "keyPick_" + keyId
            width: ListView.view.width
            leftPadding: 24
            rightPadding: 8
            enabled: !fixed
            onClicked: sheet.toggle(keyId)
            Accessible.role: Accessible.CheckBox
            Accessible.checked: chosen
            Accessible.name: Wording.keyTitle(name, email, keyId)

            contentItem: KeyChoiceFace {
                name: choice.name
                email: choice.email
                keyId: choice.keyId
                note: choice.fixed ? qsTr("Already in") : ""

                CheckBox {
                    checked: choice.chosen
                    enabled: !choice.fixed
                    focusPolicy: Qt.NoFocus
                    onToggled: sheet.toggle(choice.keyId)
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
        Button {
            text: qsTr("Cancel")
            flat: true
            onClicked: sheet.close()
        },
        Button {
            objectName: "keyPickerAccept"
            text: sheet.pickedCount > 0
                  ? qsTr("%1 (%2)").arg(sheet.actionText).arg(sheet.pickedCount)
                  : sheet.actionText
            highlighted: true
            enabled: sheet.pickedCount > 0
            onClicked: {
                const ids = Object.keys(sheet.picked)
                sheet.close()
                sheet.accepted(ids)
            }
        }
    ]
}
