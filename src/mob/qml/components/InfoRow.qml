// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One fact as a row: what it is on the left, its value on the right, the
// value wrapping in its own column however long it is (a compiler's whole
// version, a hash). With large text, or no room beside it, the value goes
// under its label instead. The value can be selected and copied whole.
GridLayout {
    id: row

    property string label
    property string value
    property bool mono: false
    property color valueColor: Material.foreground

    // Beside each other while the label takes no more than about a third.
    readonly property bool stacked: Theme.fontScale > 1.3 || width < 280

    columns: stacked ? 1 : 2
    columnSpacing: Theme.gap
    rowSpacing: 2
    visible: value.length > 0

    Label {
        objectName: "infoLabel"
        Layout.preferredWidth: row.stacked ? -1 : Math.min(implicitWidth, row.width * 0.38)
        Layout.fillWidth: row.stacked
        Layout.alignment: Qt.AlignTop
        text: row.label
        wrapMode: Text.Wrap
        color: Material.secondaryTextColor
        font.pixelSize: Theme.secondarySize
    }

    TextEdit {
        objectName: "infoValue"
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        text: row.value
        readOnly: true
        selectByMouse: true
        wrapMode: row.mono ? TextEdit.WrapAnywhere : TextEdit.Wrap
        textFormat: TextEdit.PlainText
        color: row.valueColor
        selectionColor: Theme.indicator
        selectedTextColor: Material.foreground
        font.family: row.mono ? Theme.mono : Qt.application.font.family
        font.pixelSize: Theme.secondarySize
        horizontalAlignment: row.stacked ? Text.AlignLeft : Text.AlignRight
        Accessible.role: Accessible.StaticText
        Accessible.name: row.label + ": " + row.value
    }
}
