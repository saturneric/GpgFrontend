// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Revoking a subkey or a user ID: why, in OpenPGP's words and the person's
// own, and that it cannot be taken back.
BottomSheet {
    id: sheet

    // What is revoked: a subkey, or a user ID ({uid, ...}) when
    // `forUid` is set, or the whole key when `forKey` is; `saving` for a
    // certificate kept to revoke the key with later.
    property var subkey: ({})
    property bool forUid: false
    property bool forKey: false
    property bool saving: false
    // OpenPGP's reason code, not a row: a user ID has its own two.
    property int reason: 3
    // What is revoked, as the shell's reasons know it.
    readonly property string target: forUid ? "uid" : forKey ? "key" : "subkey"

    signal accepted(var subkey, int reason, string text)

    function begin(subkey) {
        acted = false
        sheet.subkey = subkey
        forUid = false
        forKey = false
        saving = false
        reason = KeyListController.defaultRevocationReason(target)
        why.clear()
        open()
    }

    function beginForUid(uid) {
        acted = false
        sheet.subkey = uid
        forUid = true
        forKey = false
        saving = false
        reason = KeyListController.defaultRevocationReason(target)
        why.clear()
        open()
    }

    // The whole key, now (@p save false) or by a certificate saved for
    // later (@p save true).
    function beginForKey(key, save) {
        acted = false
        sheet.subkey = key
        forUid = false
        forKey = true
        saving = save === true
        reason = KeyListController.defaultRevocationReason(target)
        why.clear()
        open()
    }

    objectName: "revokeSubkeySheet"
    title: saving ? qsTr("Save a revocation certificate")
         : forKey ? qsTr("Revoke this key")
         : forUid ? qsTr("Revoke user ID")
                  : qsTr("Revoke subkey %1").arg((subkey.keyId || "").slice(-8))

    Label {
        Layout.fillWidth: true
        visible: sheet.forUid
        wrapMode: Text.WrapAnywhere
        font.weight: Font.DemiBold
        text: sheet.subkey.uid || ""
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: sheet.saving ? Material.secondaryTextColor : Theme.danger
        text: sheet.saving
              ? qsTr("Keep it apart from the key. If the key is lost or its "
                     + "passphrase forgotten, importing it elsewhere still ends "
                     + "the key's use. Anyone who has it can do that, so keep it "
                     + "safe. The reason is written into it now.")
              : sheet.forKey
              ? qsTr("A revoked key can no longer be used to encrypt to you or "
                     + "to sign, and this cannot be undone. Unlike deleting, it "
                     + "tells others: share the key afterwards, so they learn of it.")
              : sheet.forUid
              ? qsTr("A revoked user ID no longer stands for the key. Share "
                     + "the key afterwards, so others learn of it.")
              : qsTr("A revoked subkey can never be used again. Share the key "
                     + "afterwards, so others learn of it.")
    }

    // One list, edge to edge, without the sheet's spacing between its
    // choices: a row a touch target tall.
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: -Theme.sheetPad
        Layout.rightMargin: -Theme.sheetPad
        spacing: 0

        Repeater {
            // The desktop's reasons, from the shell: a whole key is
            // replaced by a newer key, not a subkey.
            model: KeyListController.revocationReasons(sheet.target)

            RadioDelegate {
                required property var modelData
                objectName: "revokeReason_" + modelData.code
                Layout.fillWidth: true
                leftPadding: Theme.sheetPad
                rightPadding: Theme.sheetPad - 8
                text: modelData.text
                // What is chosen, always: checkable, so TalkBack says which,
                // but a tap that ticks the row gives it back its binding.
                checkable: true
                autoExclusive: false
                checked: sheet.reason === modelData.code
                onToggled: checked = Qt.binding(() => sheet.reason === modelData.code)
                onClicked: sheet.reason = modelData.code
            }
        }
    }

    TextField {
        id: why
        objectName: "revokeText"
        Layout.fillWidth: true
        placeholderText: qsTr("Description (optional)")
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
            kind: sheet.saving ? "primary" : "destructive"
            objectName: "revokeRun"
            text: sheet.saving ? qsTr("Choose where") : qsTr("Revoke")
            onClicked: {
                // A second tap while it slides away: already done.
                if (sheet.acted) return
                sheet.acted = true
                sheet.close()
                sheet.accepted(sheet.subkey, sheet.reason, why.text)
            }
        }
    ]
}
