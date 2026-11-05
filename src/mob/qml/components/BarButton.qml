// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import GpgFrontend.Mobile

// An icon in a top bar: the style's 48 dp target, its text read out by
// accessibility services, and shown as a tip while it is held.
ToolButton {
    id: button

    property bool held: false

    display: AbstractButton.IconOnly
    Accessible.name: text

    onPressAndHold: held = true
    onReleased: held = false
    onCanceled: held = false

    ToolTip.visible: held
    ToolTip.text: text
}
