// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One user ID of a key, as the desktop's user ID tab shows it, with what can
// be done to it: make it the primary one, revoke it, delete it. The primary
// user ID is the key's name: it is only replaced, by making another primary.
BottomSheet {
    id: sheet

    // The user ID, as KeyActionsController.describe() lists it.
    property var uid: ({})
    // What may be done to the key's user IDs (describe()'s "can").
    property var can: ({})
    property bool busy: false

    readonly property bool primary: uid.primary === true
    readonly property bool revoked: uid.revoked === true

    signal primaryRequested(var uid)
    signal revokeRequested(var uid)
    signal deleteRequested(var uid)

    function begin(uid, can) {
        sheet.uid = uid
        sheet.can = can
        open()
    }

    objectName: "uidSheet"
    title: uid.name || uid.email || qsTr("User ID")

    component SheetAction: ListRow {
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        leftPadding: Theme.sheetPad
        symbolColor: Material.foreground
        enabled: !sheet.busy
        onClicked: sheet.close()
    }

    Label {
        objectName: "uidState"
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: sheet.revoked || sheet.uid.invalid === true ? Theme.danger : Material.foreground
        text: sheet.revoked ? qsTr("Revoked")
            : sheet.uid.invalid === true ? qsTr("Invalid")
            : sheet.primary ? qsTr("The primary user ID: the key's name")
                            : qsTr("Valid")
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 1
        rowSpacing: 8

        Fact {
            title: qsTr("Name")
            value: sheet.uid.name || ""
        }
        Fact {
            title: qsTr("Email")
            value: sheet.uid.email || ""
        }
        Fact {
            title: qsTr("Comment")
            value: sheet.uid.comment || ""
        }
        Fact {
            title: qsTr("User ID")
            value: sheet.uid.uid || ""
        }
    }

    MenuSeparator {
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        visible: makePrimary.visible || revoke.visible || remove.visible
    }

    SheetAction {
        id: makePrimary
        objectName: "uidPrimary"
        visible: sheet.can.setPrimaryUid === true && !sheet.primary && !sheet.revoked
        symbol: "verified"
        text: qsTr("Make primary")
        detail: qsTr("Others see this name first.")
        onClicked: sheet.primaryRequested(sheet.uid)
    }

    SheetAction {
        id: revoke
        objectName: "uidRevoke"
        visible: sheet.can.revokeUid === true && !sheet.primary && !sheet.revoked
        symbol: "warning"
        text: qsTr("Revoke")
        detail: qsTr("Says to everyone with the updated key that it no longer "
                     + "stands for you. This cannot be undone.")
        onClicked: sheet.revokeRequested(sheet.uid)
    }

    SheetAction {
        id: remove
        objectName: "uidDelete"
        Layout.bottomMargin: Theme.gapSmall
        visible: sheet.can.deleteUid === true && !sheet.primary
        symbol: "delete"
        symbolColor: Theme.danger
        text: qsTr("Delete")
        detail: qsTr("Only here: copies of the key others have keep it. To "
                     + "withdraw it, revoke it instead.")
        onClicked: sheet.deleteRequested(sheet.uid)
    }
}
