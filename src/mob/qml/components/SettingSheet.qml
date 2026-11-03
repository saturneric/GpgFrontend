// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// Choosing one setting's value: the values worth offering, then, where the
// setting allows it, any other.
BottomSheet {
    id: sheet

    property int row: -1
    property string unit
    property var choices: []
    // Whether a value other than the choices may be set.
    property bool custom: true
    property int value: 0

    // The value is stored; the caller writes it.
    signal chosen(int row, int value)

    // "10 minutes", "2 hours", "30 days". Spelled out per number instead of
    // qsTr("%n day(s)"): without a translator loaded Qt keeps the literal
    // "(s)", as the desktop found (KeySetExpireDateDialog).
    function describe(amount, unit) {
        if (unit === "days")
            return amount === 1 ? qsTr("1 day") : qsTr("%1 days").arg(amount)
        if (unit !== "seconds") return String(amount)
        if (amount > 0 && amount % 3600 === 0) {
            const hours = amount / 3600
            return hours === 1 ? qsTr("1 hour") : qsTr("%1 hours").arg(hours)
        }
        if (amount > 0 && amount % 60 === 0) {
            const minutes = amount / 60
            return minutes === 1 ? qsTr("1 minute")
                                 : qsTr("%1 minutes").arg(minutes)
        }
        return amount === 1 ? qsTr("1 second") : qsTr("%1 seconds").arg(amount)
    }

    function begin(row, title, unit, choices, custom, value) {
        sheet.row = row
        sheet.title = title
        sheet.unit = unit
        sheet.choices = choices || []
        sheet.custom = custom
        sheet.value = value
        other.checked = sheet.custom && sheet.choices.indexOf(value) < 0
        amount.value = value
        open()
    }

    ButtonGroup {
        id: group
    }

    Repeater {
        model: sheet.choices

        RadioDelegate {
            required property var modelData

            Layout.fillWidth: true
            Layout.leftMargin: -24
            Layout.rightMargin: -24
            leftPadding: 24
            text: sheet.describe(modelData, sheet.unit)
            checked: !other.checked && modelData === sheet.value
            ButtonGroup.group: group
            onClicked: {
                sheet.chosen(sheet.row, modelData)
                sheet.close()
            }
        }
    }

    RadioDelegate {
        id: other

        Layout.fillWidth: true
        Layout.leftMargin: -24
        Layout.rightMargin: -24
        leftPadding: 24
        text: qsTr("Other")
        visible: sheet.custom
        ButtonGroup.group: group
    }

    RowLayout {
        Layout.fillWidth: true
        visible: other.checked
        spacing: 12

        SpinBox {
            id: amount
            from: 0
            to: 1000000
            editable: true
            inputMethodHints: Qt.ImhDigitsOnly
        }

        Label {
            Layout.fillWidth: true
            text: sheet.unit === "days" ? qsTr("days")
                  : sheet.unit === "seconds" ? qsTr("seconds") : ""
            color: Material.secondaryTextColor
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
            text: qsTr("Set")
            highlighted: true
            visible: other.checked
            onClicked: {
                // What is typed, whether or not the field has committed it:
                // a button pressed on a touch screen may never take the focus.
                const typed = amount.valueFromText(amount.contentItem.text,
                                                   amount.locale)
                const value = isNaN(typed) ? amount.value
                            : Math.max(amount.from, Math.min(amount.to, typed))
                sheet.chosen(sheet.row, value)
                sheet.close()
            }
        }
    ]
}
