// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GpgFrontend.Mobile

// A setting in a sheet's form: its words at the left, the switch at the
// right, the row as wide as the sheet. Its words wrap beside the switch,
// however large the text.
SwitchDelegate {
    id: setting

    Layout.fillWidth: true
    Layout.leftMargin: -Theme.sheetPad
    Layout.rightMargin: -Theme.sheetPad
    leftPadding: Theme.sheetPad
    rightPadding: Theme.sheetPad - 8
    topPadding: 4
    bottomPadding: 4

    contentItem: Label {
        text: setting.text
        font: setting.font
        wrapMode: Text.Wrap
        verticalAlignment: Text.AlignVCenter
        rightPadding: setting.indicator.width + setting.spacing
    }
}
