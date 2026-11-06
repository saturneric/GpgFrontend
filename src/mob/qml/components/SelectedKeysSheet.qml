// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// The keys chosen to encrypt to, over the key picker: each with what is
// wrong with it, if anything, and a way to take it out. Back, or Done, goes
// back to the picker as it was; the count there follows.
BottomSheet {
    id: sheet

    signal aboutKeyRequested(var key)

    objectName: "selectedKeysSheet"
    inset: 0
    expandable: true
    listMode: true
    title: Wording.selectedCount(WorkSelection.recipientCount)

    // Nothing left to show: back to the picker.
    readonly property int count: WorkSelection.recipientCount
    onCountChanged: if (count === 0 && opened) close()

    ListView {
        id: list

        objectName: "selectedKeys"
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: contentHeight + bottomMargin
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        bottomMargin: Theme.gapSmall
        model: sheet.opened ? WorkSelection.recipientKeys : []
        ScrollIndicator.vertical: ScrollIndicator {}

        delegate: ItemDelegate {
            id: row

            required property var modelData
            readonly property string issue: modelData.issue || ""

            width: ListView.view.width
            leftPadding: Theme.sheetPad
            rightPadding: 4
            topPadding: 6
            bottomPadding: 6
            onClicked: if (issue !== "missing") sheet.aboutKeyRequested(modelData)

            contentItem: KeyChoiceFace {
                idBelow: true
                name: row.modelData.name || ""
                email: row.modelData.email || ""
                // A key no longer here has only the fingerprint it was
                // chosen by: its end stands for it.
                keyId: row.modelData.keyId || (row.modelData.fingerprint || "").slice(-16)
                isKey: row.modelData.isKey !== false
                usable: row.issue.length === 0
                members: row.modelData.members || []
                note: row.issue.length > 0 ? (row.modelData.note || "") : ""
                problem: row.issue.length > 0

                BarButton {
                    objectName: "removeSelected"
                    text: qsTr("Remove %1").arg(Wording.keyName(row.modelData))
                    icon.source: Theme.icon("close")
                    onClicked: WorkSelection.removeRecipient(row.modelData.fingerprint)
                }
            }
        }
    }

    // No Close of its own: Back, a tap beside it or pulling it down put it
    // away, as every sheet does.
}
