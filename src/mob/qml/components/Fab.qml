// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import GpgFrontend.Mobile

// A page's main action, as the platform's floating button at the bottom
// right: a rounded square of the accent, with its icon, read out by its text
// and shown as a tip while held.
RoundButton {
    id: fab

    anchors.right: parent ? parent.right : undefined
    anchors.bottom: parent ? parent.bottom : undefined
    anchors.margins: Theme.gap
    width: 56
    height: 56
    radius: 16
    highlighted: true
    display: AbstractButton.IconOnly
    Accessible.name: text

    property bool held: false

    onPressAndHold: held = true
    onReleased: held = false
    onCanceled: held = false

    ToolTip.visible: held
    ToolTip.text: text
}
