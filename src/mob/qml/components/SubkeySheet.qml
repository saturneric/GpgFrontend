// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One of a key's subkeys, or its primary key, as the desktop's subkey list
// details it, with what can be done to it here: give it another expiry date,
// revoke it, delete it. The primary key is not revoked or deleted on its own:
// that is the whole key's, on the key page.
BottomSheet {
    id: sheet

    // The subkey, as KeyActionsController.describe() lists it.
    property var subkey: ({})
    // What may be done to the key's subkeys (describe()'s "can").
    property var can: ({})
    // While an operation runs, nothing else is started.
    property bool busy: false

    readonly property bool primary: subkey.primary === true

    signal expirationRequested(var subkey)
    signal revokeRequested(var subkey)
    signal deleteRequested(var subkey)

    // The key's secretState (KeyRoleModel's): the primary key's mark.
    property string keySecretState: ""

    function begin(subkey, can) {
        sheet.subkey = subkey
        sheet.can = can
        open()
    }

    objectName: "subkeySheet"
    title: primary ? qsTr("Primary key")
                   : qsTr("Subkey %1").arg((subkey.keyId || "").slice(-8))

    // GnuPG's mark for it: sec/pub for the primary key, ssb/sub for a
    // subkey, by whether its secret is here.
    headerItems: Label {
        objectName: "subkeyTag"
        text: sheet.primary ? Wording.keyTag(sheet.keySecretState)
                            : Wording.subkeyTag(sheet.subkey.secret === true,
                                                sheet.subkey.card === true)
        font.family: Theme.mono
        font.pixelSize: Theme.secondarySize
        color: Material.accentColor
    }

    component SheetAction: ListRow {
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        leftPadding: Theme.sheetPad
        symbolColor: Material.foreground
        enabled: !sheet.busy
        onClicked: sheet.close()
    }

    // Only when it is no longer usable: its expiration date is a fact below.
    Label {
        objectName: "subkeyState"
        Layout.fillWidth: true
        visible: sheet.subkey.revoked === true || sheet.subkey.expired === true
        wrapMode: Text.Wrap
        text: Wording.keyState(sheet.subkey)
        color: sheet.subkey.revoked === true || sheet.subkey.expired === true
               ? Theme.danger : Material.foreground
    }

    FingerprintView {
        Layout.fillWidth: true
        Layout.topMargin: 8
        fingerprint: sheet.subkey.fingerprint || ""
    }

    KeyPartFacts {
        Layout.fillWidth: true
        Layout.topMargin: 8
        part: sheet.subkey
        kindTitle: qsTr("Type")
        kindValue: sheet.primary ? qsTr("Primary key")
                 : sheet.subkey.adsk === true ? qsTr("Additional decryption subkey")
                                              : qsTr("Subkey")
        expiresText: Theme.date(sheet.subkey.expires) || qsTr("Never")
        secretText: sheet.subkey.secret === true ? qsTr("On this device")
                                                 : qsTr("Not here")
        cardText: sheet.subkey.card === true ? qsTr("On a smart card") : ""
    }

    MenuSeparator {
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        Layout.topMargin: 8
        visible: expiry.visible || revoke.visible || remove.visible
    }

    SheetAction {
        id: expiry
        objectName: "subkeyExpiration"
        visible: sheet.can.setExpiration === true && sheet.subkey.revoked !== true
        symbol: "settings"
        text: qsTr("Change expiration")
        onClicked: sheet.expirationRequested(sheet.subkey)
    }

    SheetAction {
        id: revoke
        objectName: "subkeyRevoke"
        visible: sheet.can.revokeSubkey === true && !sheet.primary
                 && sheet.subkey.revoked !== true
        symbol: "warning"
        text: qsTr("Revoke")
        detail: qsTr("No one can use it once they have the updated key. "
                     + "This cannot be undone.")
        onClicked: sheet.revokeRequested(sheet.subkey)
    }

    SheetAction {
        id: remove
        objectName: "subkeyDelete"
        Layout.bottomMargin: Theme.gapSmall
        visible: sheet.can.deleteSubkey === true && !sheet.primary
        symbol: "delete"
        symbolColor: Theme.danger
        text: qsTr("Delete")
        detail: qsTr("Only here: copies of the key others have keep it. To "
                     + "end its use, revoke it instead.")
        onClicked: sheet.deleteRequested(sheet.subkey)
    }
}
