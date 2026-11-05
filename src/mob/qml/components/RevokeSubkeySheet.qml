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
    // `forUid` is set.
    property var subkey: ({})
    property bool forUid: false
    // OpenPGP's reason code, not a row: a user ID has its own two.
    property int reason: 3

    signal accepted(var subkey, int reason, string text)

    function begin(subkey) {
        sheet.subkey = subkey
        forUid = false
        reason = 3
        why.clear()
        open()
    }

    function beginForUid(uid) {
        sheet.subkey = uid
        forUid = true
        // "No longer valid", as the desktop starts.
        reason = 4
        why.clear()
        open()
    }

    objectName: "revokeSubkeySheet"
    title: forUid ? qsTr("Revoke user ID")
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
        color: Theme.danger
        text: sheet.forUid
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
            model: sheet.forUid ? Wording.uidRevocationReasons()
                                : Wording.revocationReasons()

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
        Button {
            text: qsTr("Cancel")
            flat: true
            onClicked: sheet.close()
        },
        Button {
            objectName: "revokeRun"
            text: qsTr("Revoke")
            highlighted: true
            Material.accent: Theme.danger
            onClicked: {
                sheet.close()
                sheet.accepted(sheet.subkey, sheet.reason, why.text)
            }
        }
    ]
}
