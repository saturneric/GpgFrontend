// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Checking a detached signature: the file it was started from is one half,
// and the other half is chosen from the list. Which of the two is the
// signature is only guessed from the name, and can be swapped; whether it
// really is one, the check says.
BottomSheet {
    id: sheet

    property FileWorkspace workspace
    property int fileId: -1
    property string fileName
    // Whether the file it was started from is the signature.
    property bool startedFromSignature: true

    function begin(id) {
        const file = workspace.entry(id)
        sheet.fileId = id
        sheet.fileName = file.name || ""
        sheet.startedFromSignature = /\.(sig|asc)$/i.test(sheet.fileName)
        open()
    }

    objectName: "verifySheet"
    title: qsTr("Verify a signature")

    Label {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        text: sheet.startedFromSignature
              ? qsTr("Choose the file “%1” is the signature of.")
                .arg(sheet.fileName)
              : qsTr("Choose the signature of “%1”.").arg(sheet.fileName)
    }

    SwitchDelegate {
        objectName: "verifyIsSignature"
        Layout.fillWidth: true
        Layout.leftMargin: -24
        Layout.rightMargin: -24
        leftPadding: 24
        rightPadding: Theme.gap
        text: qsTr("“%1” is the signature").arg(sheet.fileName)
        checked: sheet.startedFromSignature
        onToggled: sheet.startedFromSignature = checked
    }

    Repeater {
        model: sheet.workspace

        ListRow {
            required property int fileId
            required property string name
            required property real size
            required property bool available

            Layout.fillWidth: true
            Layout.leftMargin: -24
            Layout.rightMargin: -24
            leftPadding: 24
            visible: fileId !== sheet.fileId
            enabled: available
            singleLineTitle: true
            symbol: "draft"
            text: name
            detail: Theme.fileSize(size)
            onClicked: {
                sheet.close()
                if (sheet.startedFromSignature)
                    sheet.workspace.verify(sheet.fileId, fileId)
                else
                    sheet.workspace.verify(fileId, sheet.fileId)
            }
        }
    }

    footer: [
        Item {
            Layout.fillWidth: true
        },
        Button {
            text: qsTr("Cancel")
            flat: true
            onClicked: sheet.close()
        }
    ]
}
