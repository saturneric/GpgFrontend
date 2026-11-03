// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GpgFrontend.Mobile

BottomSheet {
    id: sheet

    signal confirmed(string signer)

    function begin() {
        keys.chosen = []
        open()
    }

    objectName: "signerSheet"
    title: qsTr("Sign with")

    KeyChoiceList {
        id: keys
        Layout.fillWidth: true
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
            text: qsTr("Sign")
            highlighted: true
            enabled: keys.chosen.length === 1
            onClicked: {
                sheet.close()
                sheet.confirmed(keys.chosen[0])
            }
        }
    ]
}
