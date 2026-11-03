// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import GpgFrontend.Mobile

// An icon in a top bar: a standard 48 dp target, its text read out by
// accessibility services, and shown as a tip while it is held.
ToolButton {
    id: button

    property bool held: false

    implicitWidth: Math.max(48, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(48, implicitContentHeight + topPadding + bottomPadding)
    display: AbstractButton.IconOnly
    Accessible.name: text

    onPressAndHold: held = true
    onReleased: held = false
    onCanceled: held = false

    ToolTip.visible: held
    ToolTip.text: text
}
