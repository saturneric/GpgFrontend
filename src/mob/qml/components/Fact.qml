// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import GpgFrontend.Mobile

// One fact of a key, in a grid of them: what it is, small, over its value.
// Left out when there is no value.
ColumnLayout {
    id: fact

    property alias title: titleLabel.text
    property alias value: valueLabel.text
    property bool mono: false
    // Across the whole grid: a key ID is not broken over two lines.
    property bool wide: false
    property color valueColor: Material.foreground

    Layout.fillWidth: true
    // Equal columns, whatever each value's length.
    Layout.preferredWidth: wide ? -1 : 1
    Layout.alignment: Qt.AlignTop
    Layout.columnSpan: wide && parent && parent.columns > 1 ? parent.columns : 1
    spacing: 2
    visible: value.length > 0

    Label {
        id: titleLabel
        color: Material.secondaryTextColor
        font.pixelSize: Theme.labelSize
    }

    Label {
        id: valueLabel
        Layout.fillWidth: true
        // Hex breaks anywhere; words only between them.
        wrapMode: fact.mono ? Text.WrapAnywhere : Text.Wrap
        color: fact.valueColor
        font.family: fact.mono ? Theme.mono : Qt.application.font.family
        font.pixelSize: Theme.bodySize
    }
}
