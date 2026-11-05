// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// How long a key or subkey lasts from now, chosen from the durations the
// desktop offers ("1 year" ... "Never").
BottomSheet {
    id: sheet

    // [{id, name}], as KeyActionsController.validities gives them.
    property var choices: []
    property string chosen: "2y"
    // What it is for, under the title.
    property string about: ""

    signal accepted(string validity)

    function begin(title, about) {
        sheet.title = title
        sheet.about = about
        chosen = "2y"
        open()
    }

    objectName: "validitySheet"
    inset: 0
    description: about

    // One list, edge to edge, without the sheet's spacing between its
    // choices: a row a touch target tall.
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: sheet.choices

            RadioDelegate {
                required property var modelData
                Layout.fillWidth: true
                leftPadding: Theme.sheetPad
                rightPadding: Theme.sheetPad - 8
                text: modelData.name
                // What is chosen, always: checkable, so TalkBack says which,
                // but a tap that ticks the row gives it back its binding.
                checkable: true
                autoExclusive: false
                checked: sheet.chosen === modelData.id
                onToggled: checked = Qt.binding(() => sheet.chosen === modelData.id)
                onClicked: sheet.chosen = modelData.id
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
        },
        Button {
            objectName: "validityApply"
            text: qsTr("Apply")
            highlighted: true
            onClicked: {
                sheet.close()
                sheet.accepted(sheet.chosen)
            }
        }
    ]
}
