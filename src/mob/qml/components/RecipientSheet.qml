// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

BottomSheet {
    id: sheet

    signal confirmed(var recipients, string signer)

    function begin() {
        recipients.chosen = []
        signer.chosen = []
        sign.checked = false
        open()
    }

    objectName: "recipientSheet"
    title: qsTr("Encrypt to")

    KeyChoiceList {
        id: recipients
        objectName: "recipientList"
        Layout.fillWidth: true
        onlyEncryptCapable: true
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        visible: recipients.chosen.length === 0
        text: qsTr("Nobody chosen: you will set a passphrase, and anyone "
                   + "with it can read the message.")
    }

    Switch {
        id: sign
        Layout.fillWidth: true
        Layout.leftMargin: -8
        text: qsTr("Sign it too")
    }

    KeyChoiceList {
        id: signer
        Layout.fillWidth: true
        visible: sign.checked
        multiple: false
        onlySecret: true
        onlySignCapable: true
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
            objectName: "recipientEncrypt"
            text: recipients.chosen.length === 0
                  ? qsTr("Encrypt with a passphrase") : qsTr("Encrypt")
            highlighted: true
            enabled: !sign.checked || signer.chosen.length === 1
            onClicked: {
                sheet.close()
                sheet.confirmed(recipients.chosen,
                                sign.checked ? signer.chosen[0] : "")
            }
        }
    ]
}
